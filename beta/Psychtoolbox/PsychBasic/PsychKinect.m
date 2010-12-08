function varargout = PsychKinect(varargin)
% PsychKinect -- Control and access the Microsoft XBOX360-Kinect.
%
% This is a high level driver to allow convenient access to the Microsoft
% XBOX-360 Kinect box. The Kinect is a depth-sensing "3D camera". The
% Kinect consists of a standard color camera (like any standard USB webcam)
% to capture a scene at 640x480 pixels resolution in RGB8 color with up to
% 30 frames per second. In addition it has a depth sensor that measures the
% distance of each "pixel" from the camera. The Kinect delivers a color
% image with depth information which can be used to infer the 3D structure
% of the observed visual scene and to perform a 3D reconstruction of the
% scene.
%
% See KinectDemo and Kinect3DDemo for two demos utilizing PsychKinect to
% demonstrate this basic functions of the Kinect.
%
% PsychKinect internally uses the PsychKinectCore MEX file which actually
% interfaces with the Kinect.
%
% The driver is currently supported on Microsoft Windows under GNU/Octave
% and Matlab version 7.4 (R2007a) and later. It is also supported on
% GNU/Linux with Matlab or Octave and on Intel based Macintosh computers
% under OS/X with Matlab or Octave. However, while Linux and Windows
% support has been extensively tested, MacOS/X support is completely
% untested due to technical difficulties and may or may not work at all.
%
% To use this driver you need:
% 1. A Microsoft Kinect (price tag about 150$ at December 2010).
% 2. A interface cable to connect the Kinect to a standard USB port of a
%    computer - sold separately or included in standalone Kinects. The
%    Kinect was meant to connect only to the XBOX via a proprietary
%    connector, therefore the need for a special cable.
% 3. The free and open-source libfreenect + libusb libraries and drivers
%    from the OpenKinect project (Homepage: http://www.openkinect.org )
%
% You need to install these libraries separately, otherwise our driver will
% abort with an "Invalid MEX file" error.
%
% Type "help InstallKinect" for installation instructions and licensing
% terms.
%
% This driver is early beta quality. It may contain significant bugs or
% missing functionality. The underlying freenect libraries are also very
% young and may contain significant bugs and limitations, so prepare for an
% exciting but potentially bumpy ride if you want to try it.
%
%
% Subfunctions:
%
% Most functions are part of the PsychKinectCore() mex file, so you can get
% help for them by typing PsychKinectCore FUNCTIONNAME ? as usual, with
% FUNCTIONNAME being the name of the function you want to get help for.
%
%
% kobject = PsychKinect('CreateObject', window, kinect [, oldkobject]);
% - Create a new kobject for the specified 'window', using the Kinect box
% specified by the given 'kinect' handle. Recycle 'oldkobject' so save
% memory and resources if 'oldkobject' is provided. Otherwise create a new
% object.
%
% Do not use within creen('BeginOpenGL', window); and Screen('EndOpenGL',
% window); calls, as 2D mode is needed.
%
%
% kobject encodes the 3D geometry of the scene as sensed by the Kinect. It
% corresponds to the currently selected "3d video frame" from the kinect,
% as selected by PsychKinect('GrabFrame', kinect);
% kobject can then by accessed directly (it is a struct variable) or passed
% to other PsychKinect functions for display and processing.
%
% PsychKinect('DeleteObject', window, kobject);
% - Delete given 'kobject' for given 'window' once you no longer need it.
% During a work-loop you could also pass 'kobject' to the next
% PsychKinect('CreateObject', ...); call as 'oldkobject' to recycle it for
% reasons of computational efficiency.
%
% Do not use within creen('BeginOpenGL', window); and Screen('EndOpenGL',
% window); calls, as 2D mode is needed.
%
%
% PsychKinect('DrawObject', window, kobject [, drawtype=0]);
% Draw the 3D scene stored in 'kobject' into the 'window' selected. Use the
% current OpenGL settings for this. 'drawtype' is optional and defines kind
% of rendering: 0 (the default) draws a colored point-cloud of all sensed
% 3D points. 1 draws a dense textured 3D surface mesh.
%
% This function must be enclosed between Screen('BeginOpenGL', window);
% and Screen('EndOpenGL', window); calls, as 3D mode is needed.
%

% History:
%  5.12.2010  mk   Initial version written.

global GL;

persistent kinect_opmode;
persistent glsl;
persistent idxvbo;
persistent idxbuffersize;

kinect_opmode = 3;

% Command specified? Otherwise we output the help text of us and
% the low level driver:
if nargin > 0
    cmd = varargin{1};
else
    help PsychKinect;
    PsychKinectCore;
    return;
end

if strcmpi(cmd, 'CreateObject')

    if nargin < 2 || isempty(varargin{2})
        error('You must provide a valid "window" handle as 1st argument!');
    end
    win = varargin{2};

    if nargin < 3 || isempty(varargin{3})
        error('You must provide a valid "kinect" handle as 2nd argument!');
    end
    kinect = varargin{3};

    if nargin < 4 || isempty(varargin{4})
        kmesh.tex = [];
        kmesh.vbo = [];
        kmesh.buffersize = 0;
    else
        kmesh = varargin{4};
    end

    kmesh.xyz = [];
    kmesh.rgb = [];

    % Turn RGB video camera image into a Psychtoolbox texture and corresponding
    % OpenGL rectangle texture:
    [imbuff, width, height, channels] = PsychKinectCore('GetImage', kinect, 0, 1);
    if width > 0 && height > 0
        kmesh.tex = Screen('SetOpenGLTextureFromMemPointer', win, kmesh.tex, imbuff, width, height, channels, 1, GL.TEXTURE_RECTANGLE_EXT);
        [ gltexid gltextarget ] =Screen('GetOpenGLTexture', win, kmesh.tex);
        kmesh.gltexid = gltexid;
        kmesh.gltextarget = gltextarget;
    else
        varargout{1} = [];
        fprintf('PsychKinect: WARNING: Failed to fetch RGB image data!\n');
        return;
    end

    if kinect_opmode == 0
        % Dumb mode: Return a complete matrix with encoded vertex positions
        % and vertex colors:
        [foo, width, height, channels, glformat] = PsychKinect('GetDepthImage', kinect, 2, 0);
        foo = reshape (foo, 6, size(foo,2) * size(foo,3));
        kmesh.xyz = foo(1:3, :);
        kmesh.rgb = foo(4:6, :);
        kmesh.type = 0;
        kmesh.glformat = glformat;
    end

    if kinect_opmode == 1
        % Fetch databuffer with preformatted data for a VBO that
        % contains interleaved (vx,vy,vz) 3D vertex positions and (tx,ty)
        % 2D texture coordinates, i.e., (vx,vy,vz,tx,ty) per element:
        [vbobuffer, width, height, channels, glformat] = PsychKinect('GetDepthImage', kinect, 3, 1);
        if width > 0 && height > 0
            Screen('BeginOpenGL', win);
            if isempty(kmesh.vbo)
                kmesh.vbo = glGenBuffers(1);
            end
            glBindBuffer(GL.ARRAY_BUFFER, kmesh.vbo);
            kmesh.buffersize = width * height * channels * 8;
            glBufferData(GL.ARRAY_BUFFER, kmesh.buffersize, vbobuffer, GL.STREAM_DRAW);
            glBindBuffer(GL.ARRAY_BUFFER, 0);
            Screen('EndOpenGL', win);
            kmesh.Stride = channels * 8;
            kmesh.textureOffset = 3 * 8;
            kmesh.nrVertices = width * height;
            kmesh.type = 1;
            kmesh.glformat = glformat;
        else
            varargout{1} = [];
            fprintf('PsychKinect: WARNING: Failed to fetch VBO geometry data!\n');
            return;
        end
    end

    if kinect_opmode == 2 || kinect_opmode == 3
        if isempty(glsl)
            % First time init of shader:

            % Fetch all camera calibration parameters from PsychKinectCore for this kinect:
            [depthsIntrinsics, rgbIntrinsics, R, T, depthsUndistort, rgbUndistort] = PsychKinectCore('SetBaseCalibration', kinect);
            [fx_d, fy_d, cx_d, cy_d] = deal(depthsIntrinsics(1), depthsIntrinsics(2), depthsIntrinsics(3), depthsIntrinsics(4));
            [fx_rgb, fy_rgb, cx_rgb, cy_rgb] = deal(rgbIntrinsics(1), rgbIntrinsics(2), rgbIntrinsics(3), rgbIntrinsics(4));
            [k1_d, k2_d, p1_d, p2_d, k3_d] = deal(depthsUndistort(1), depthsUndistort(2), depthsUndistort(3), depthsUndistort(4), depthsUndistort(5));
            [k1_rgb, k2_rgb, p1_rgb, p2_rgb, k3_rgb] = deal(rgbUndistort(1), rgbUndistort(2), rgbUndistort(3), rgbUndistort(4), rgbUndistort(5));

            if kinect_opmode == 2
                % Standard shader: Doesn't do initial sensor -> depths conversion.
                glsl = LoadGLSLProgramFromFiles('KinectShaderStandard');
            else
                % Compressed shader: Does this first step as well, albeit only with
                % single precision float precision instead of the double precision of
                % the C implementation. Drastically faster and no perceptible difference,
                % but that doesn't mean there isn't any:
                glsl = LoadGLSLProgramFromFiles('KinectShaderCompressed');
            end

            % Assign all relevant camera parameters to shader: Optical undistortion data isn't
            % used yet, but would be easy to do at least for the rgb camera, within a fragment
            % shader:
            glUseProgram(glsl);
            glUniform4f(glGetUniformLocation(glsl, 'depth_intrinsic'), fx_d, fy_d, cx_d, cy_d);
            glUniform4f(glGetUniformLocation(glsl, 'rgb_intrinsic'), fx_rgb, fy_rgb, cx_rgb, cy_rgb);
            glUniformMatrix3fv(glGetUniformLocation(glsl, 'R'), 1, GL.TRUE, R);
            glUniform3fv(glGetUniformLocation(glsl, 'T'), 1, T);
            glUseProgram(0);
            repeatedscan = 0;
        else
            repeatedscan = 1;
        end

        if isempty(idxvbo)
            toponame = [PsychtoolboxConfigDir 'kinect_quadmeshtopology.mat'];
            if exist(toponame,'file')
                load(toponame);
            else
                tic;
                fprintf('Building mesh topology. This is a one time effort that can take some seconds. Please standby...\n');
                % Build static fixed mesh topology for a GL_QUADS style mesh:
                meshindices = uint32(zeros(4, 639, 479));
                for yi=1:479
                    for xi=1:639
                        meshindices(1, xi, yi) = (yi-1+0) * 640 + (xi-1+0);
                        meshindices(2, xi, yi) = (yi-1+0) * 640 + (xi-1+1);
                        meshindices(3, xi, yi) = (yi-1+1) * 640 + (xi-1+1);
                        meshindices(4, xi, yi) = (yi-1+1) * 640 + (xi-1+0);
                    end
                end
                save(toponame, '-V6', 'meshindices');
                fprintf('...done. Saved to file to save startup time next time you use me. Took at total of %f seconds.\n\n', toc);
            end

            idxvbo = glGenBuffers(1);
            glBindBuffer(GL.ELEMENT_ARRAY_BUFFER, idxvbo);
            idxbuffersize = 479 * 639 * 4 * 4;
            glBufferData(GL.ELEMENT_ARRAY_BUFFER, idxbuffersize, meshindices, GL.STATIC_DRAW);
            glBindBuffer(GL.ELEMENT_ARRAY_BUFFER, 0);
        end

        % Fetch databuffer with preformatted data for a VBO that
        % contains interleaved (x,y,vz) 3D vertex positions:
        if kinect_opmode == 2
            format = 4 + repeatedscan;
        else
            % Opmode 3 outsources computation of raw depths from raw sensor data to the
            % Vertex shader as well, maybe with slightly reduced precision:
            format = 6 + repeatedscan;
        end

        [vbobuffer, width, height, channels, glformat] = PsychKinect('GetDepthImage', kinect, format, 1);
        if width > 0 && height > 0
            Screen('BeginOpenGL', win);
            if isempty(kmesh.vbo)
                kmesh.vbo = glGenBuffers(1);
            end
            glBindBuffer(GL.ARRAY_BUFFER, kmesh.vbo);
            kmesh.buffersize = width * height * channels * 8;
            glBufferData(GL.ARRAY_BUFFER, kmesh.buffersize, vbobuffer, GL.STREAM_DRAW);
            glBindBuffer(GL.ARRAY_BUFFER, 0);
            Screen('EndOpenGL', win);
            kmesh.Stride = channels * 8;
            kmesh.textureOffset = 0;
            kmesh.nrVertices = width * height;
            kmesh.type = kinect_opmode;
            kmesh.glsl = glsl;
            kmesh.glformat = glformat;
        else
            varargout{1} = [];
            fprintf('PsychKinect: WARNING: Failed to fetch VBO geometry data!\n');
            return;
        end
    end

    % Store handle to kinect:
    kmesh.kinect = kinect;
    kmesh.idxvbo = idxvbo;
    kmesh.idxbuffersize = idxbuffersize;
    varargout{1} = kmesh;

    return;
end

if strcmpi(cmd, 'DeleteObject')
    if nargin < 2 || isempty(varargin{2})
        error('You must provide a valid "window" handle as 1st argument!');
    end
    win = varargin{2};

    if nargin < 3 || isempty(varargin{3})
        error('You must provide a valid "mesh" struct as 2nd argument!');
    end
    kmesh = varargin{3};

    if ~isempty(kmesh.tex)
        Screen('Close', kmesh.tex);
    end

    if kmesh.type == 0
        return;
    end

    if kmesh.type == 1 || kmesh.type == 2 || kmesh.type == 3
        if ~isempty(kmesh.vbo)
            glDeleteBuffers(1, kmesh.vbo);
        end
        return;
    end

    return;
end

if strcmpi(cmd, 'RenderObject')

    if nargin < 2 || isempty(varargin{2})
        error('You must provide a valid "window" handle as 1st argument!');
    end
    win = varargin{2};

    if nargin < 3 || isempty(varargin{3})
        error('You must provide a valid "mesh" struct as 2nd argument!');
    end
    kmesh = varargin{3};

    if nargin < 4 || isempty(varargin{4})
        drawtype = 0;
    else
        drawtype = varargin{4};
    end

    % Primitive way: All data encoded inside kmesh.xyz, kmesh.rgb as
    % double matrices. Use PTB function to draw it. Sloooow:
    if kmesh.type == 0
        moglDrawDots3D(win, kmesh.xyz, 2, kmesh.rgb, [], 1);
    end

    % VBO with encoded texture coordinates?
    if kmesh.type == 1
        % Yes. No need for GPU post-processing, just bind & draw:

        glPushAttrib(GL.ALL_ATTRIB_BITS);

        % Activate and bind texture on unit 0:
        glActiveTexture(GL.TEXTURE0);
        glEnable(kmesh.gltextarget);
        glBindTexture(kmesh.gltextarget, kmesh.gltexid);

        % Textures color texel values shall modulate the color computed by lighting model:
        glTexEnvfv(GL.TEXTURE_ENV,GL.TEXTURE_ENV_MODE,GL.MODULATE);
        glColor3f(1,1,1);

        % Use alpha blending, so fragment alpha has desired effect:
        glEnable(GL.BLEND);
        glBlendFunc(GL.SRC_ALPHA, GL.ONE_MINUS_SRC_ALPHA);

        % Activate and bind VBO:
        glEnableClientState(GL.VERTEX_ARRAY);
        glBindBuffer(GL.ARRAY_BUFFER, kmesh.vbo);
        glVertexPointer(3, GL.DOUBLE, kmesh.Stride, 0);
        glEnableClientState(GL.TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL.DOUBLE, kmesh.Stride, kmesh.textureOffset);

        % Pure point cloud rendering requested?
        if drawtype == 0
            glDrawArrays(GL.POINTS, 0, kmesh.nrVertices);
        end

        if drawtype == 1
            glBindBuffer(GL.ELEMENT_ARRAY_BUFFER, kmesh.idxvbo);
            glDrawRangeElements(GL.QUADS, 0, 479 * 640 + 639, kmesh.idxbuffersize / 4, GL.UNSIGNED_INT, 0);
            glBindBuffer(GL.ELEMENT_ARRAY_BUFFER, 0);
        end

        glBindBuffer(GL.ARRAY_BUFFER, 0);
        glDisableClientState(GL.VERTEX_ARRAY);
        glDisableClientState(GL.TEXTURE_COORD_ARRAY);
        glBindTexture(kmesh.gltextarget, 0);
        glDisable(kmesh.gltextarget);
        glPopAttrib;
    end

    if kmesh.type == 2 || kmesh.type == 3
        % Yes. Need for GPU post-processing:

        % Activate and bind texture on unit 0:
        glPushAttrib(GL.ALL_ATTRIB_BITS);
        glActiveTexture(GL.TEXTURE0);
        glEnable(kmesh.gltextarget);
        glBindTexture(kmesh.gltextarget, kmesh.gltexid);

        % Textures color texel values shall modulate the color computed by lighting model:
        glTexEnvfv(GL.TEXTURE_ENV,GL.TEXTURE_ENV_MODE,GL.MODULATE);
        glColor3f(1,1,1);

        % Use alpha blending, so fragment alpha has desired effect:
        glEnable(GL.BLEND);
        glBlendFunc(GL.SRC_ALPHA, GL.ONE_MINUS_SRC_ALPHA);

        % Activate and bind VBO:
        glEnableClientState(GL.VERTEX_ARRAY);
        glBindBuffer(GL.ARRAY_BUFFER, kmesh.vbo);
        if kmesh.type == 3
            glVertexPointer(2, kmesh.glformat, kmesh.Stride, 0);
        else
            glVertexPointer(3, kmesh.glformat, kmesh.Stride, 0);
        end
        glUseProgram(kmesh.glsl);

        % Pure point cloud rendering requested?
        if drawtype == 0
            glDrawArrays(GL.POINTS, 0, kmesh.nrVertices);
        end

        if drawtype == 1
            glBindBuffer(GL.ELEMENT_ARRAY_BUFFER, kmesh.idxvbo);
            glDrawRangeElements(GL.QUADS, 0, 479 * 640 + 639, kmesh.idxbuffersize / 4, GL.UNSIGNED_INT, 0);
            glBindBuffer(GL.ELEMENT_ARRAY_BUFFER, 0);
        end

        glUseProgram(0);
        glBindBuffer(GL.ARRAY_BUFFER, 0);
        glDisableClientState(GL.VERTEX_ARRAY);
        glBindTexture(kmesh.gltextarget, 0);
        glDisable(kmesh.gltextarget);
        glPopAttrib;
    end

    return;
end

% No matching command found: Pass all arguments to the low-level
% PsychKinectCore mex file driver. Low level command might be
% implemented there:
[ varargout{1:nargout} ] = PsychKinectCore(varargin{:});

return;

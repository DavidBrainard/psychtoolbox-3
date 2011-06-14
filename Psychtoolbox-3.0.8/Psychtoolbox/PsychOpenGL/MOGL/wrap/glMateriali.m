function glMateriali( face, pname, param )

% glMateriali  Interface to OpenGL function glMateriali
%
% usage:  glMateriali( face, pname, param )
%
% C function:  void glMateriali(GLenum face, GLenum pname, GLint param)

% 05-Mar-2006 -- created (generated automatically from header files)

if nargin~=3,
    error('invalid number of arguments');
end

moglcore( 'glMateriali', face, pname, param );

return

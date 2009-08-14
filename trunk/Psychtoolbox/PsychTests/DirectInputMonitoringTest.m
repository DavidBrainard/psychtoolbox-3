function DirectInputMonitoringTest
% DirectInputMonitoringTest - Test "Zero latency direct input monitoring" feature of some sound cards.
%
% This test will currently only work on a subset of some ASIO 2.0 capable
% soundcards on Microsoft Windows operating systems with the latest
% PsychPortAudio driver and portaudio_x86 ASIO enabled plugin from the
% Psychtoolbox Wiki.
%
% The test allows you to exercise Direct input monitoring features on your
% soundcard if the card supports them.
%
% Use ESCape to exit. Space to toggle mute/unmute. 'i' key to change input
% channel to monitor, 'o' key to select output channel for monitoring. Use
% Cursor Up/Down to increase and decrease amplifier gain. Use Left/Right
% cursor keys to change stereo panning.
%

% History:
% 2.8.2009  mk  Written.

% Open auto-detected audio device in full-duplex mode for audio capture &
% playback, with lowlatency mode 1. Lowlatency mode is not strictly
% required, but has the nice side-effect to automatically assing the lowest
% latency audio device, which is an installed ASIO card on Windows or the
% ALSA audio system on Linux -- both are required for direct input
% monitoring to work:
pa = PsychPortAudio('Open', [], 1+2, 1, 44100, 2);

% Start with safe assumption of 2 input/output channels:
ninputs = 2;
noutputs = 2;

% Query all ASIO devices on Windows and assign in/outchannelcount if
% possible:
if IsWin
    devs = PsychPortAudio('GetDevices', 3);
    if ~isempty(devs)
        devs = devs(1);
        ninputs = devs.NrInputChannels;
        noutputs = devs.NrOutputChannels;
    end
end

% Select all inputchannels (-1) for monitoring. Could also spec a specific
% channel number >=0 to set monitoring settings on a per-channel basis:
inputchannel = -1;

% Output to channel 0 or channels 0 & 1 on a stereo output channel pair.
% Any even number would do:
outputchannel = 0;

% Start with 0 dB gain: Values between -1 and +1 are valid on ASIO hardware
% for attenuation (-1 = -inf dB) or amplification (+1 = +12 dB).
gain = 0.0;

% Sterat with centered output on a stero channel: Values between 0.0 and
% 1.0 select left <-> right stereo panning, 0.5 is centered:
pan = 0.5;

KbName('UnifyKeyNames');
lKey = KbName('LeftArrow');
rKey = KbName('RightArrow');
uKey = KbName('UpArrow');
dKey = KbName('DownArrow');
oKey = KbName('o');
iKey = KbName('i');
space = KbName('space');
esc = KbName('ESCAPE');

KbReleaseWait;
unmute = 1;

% Set initial 'DirectInputMonitoring mode':
diResult = PsychPortAudio('DirectInputMonitoring', pa, unmute, inputchannel, outputchannel, gain, pan);

% Repeat parameter change loop until user presses ESCape or error:
while diResult == 0
    % Wait for user keypress:
    [secs, keyCode] = KbStrokeWait;
    
    % Disable old setting - Mute current configuration:
    % Don't know if this is really needed or not, but let's start safe...
    diResult = PsychPortAudio('DirectInputMonitoring', pa, 0, inputchannel, outputchannel, gain, pan);
    if diResult > 0
        % Exit if unsupported or error:
        break;
    end
    
    if keyCode(esc)
        % Exit:
        break;
    end
    
    if keyCode(space)
        % Mute/Unmute:
        unmute = 1 - unmute;
    end

    % Change stereo panning left/right:
    if keyCode(lKey)
        pan = min(1.0, max(0.0, pan - 0.1));
    end

    if keyCode(rKey)
        pan = min(1.0, max(0.0, pan + 0.1));
    end
    
    % Change gain between -inf dB (Mute) to +12 dB on ASIO hardware:
    if keyCode(uKey)
        gain = min(1.0, max(-1.0, gain + 0.1));
    end

    if keyCode(dKey)
        gain = min(1.0, max(-1.0, gain - 0.1));
    end
    
    % Switch through possible output channels:
    if keyCode(oKey)
        % Always increment by two so outputchannel stays even, which is
        % required for ASIO hardware:
        outputchannel = mod(outputchannel + 2, noutputs);
    end
    
    % Switch through possible output channels:
    if keyCode(iKey)
        inputchannel = inputchannel + 1;
        % Wrap around to -1 (all channels) if max. reached:
        if inputchannel >= ninputs
            inputchannel = -1;
        end
    end
    
    % Set a new 'DirectInputMonitoring mode':
    diResult = PsychPortAudio('DirectInputMonitoring', pa, unmute, inputchannel, outputchannel, gain, pan);
    fprintf('Unmuted: %i Inchannel: %i, OutChannel: %i, gain %f, stereopan %f, RC = %i\n', unmute, inputchannel, outputchannel, gain, pan, diResult);
end

% Done. Try to mute setup. Don't care about error flag...
PsychPortAudio('DirectInputMonitoring', pa, 0, inputchannel, outputchannel, gain, pan);

% Close device and driver:
PsychPortAudio('Close');
fprintf('Done. Bye!\n');

return;

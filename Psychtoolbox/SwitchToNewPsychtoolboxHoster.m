function SwitchToNewPsychtoolboxHoster
% Switch a installed Psychtoolbox working copy to our new host GoogleCode.
%
% This function checks if the currently installed Psychtoolbox working copy
% is still attached to Berlios code hosting. If so, it tries to switch it
% over to our new code hosting provider GoogleCode. Otherwise it does
% nothing.
%
% This function is called by the PsychtoolboxPostInstallRoutine after each
% invocation of DownloadPsychtoolbox, UpdatePsychtoolbox or SetupPsychtoolbox.
% It takes care of the transition.
%
% The function can also be called manually by users in the year 2012 and
% later, when the standard installation and update routines do no longer
% work due to Berlios servers being offline.
%

% History:
% 31.10.2011  mk  Written.
% 08.01.2012  mk  Disable the copy of the routine hosted at GoogleCode,
%                 as its execution would be entirely redundant by definition.

% We can skip this routine. Why? Because all current copies of the routine
% We are hosted on GoogleCode since 2012, so the pure fact that this version
% of the file is executed means that we *are* in fact hosted on GoogleCode.
% Therefore no need for a redundant check :)
% The write-locked copy of this routine on Berlios is still active and
% important for switchovers.

% Use this version of the routine as a reminder that users should get
% up-to-date versions of PTB 3.0.10+ from our new GitHub repo:
fprintf('\n\n\n\n');
fprintf('You have just updated to a old legacy version of Psychtoolbox.\n');
fprintf('This is a version older than version 3.0.10. It is no longer\n');
fprintf('supported or maintained. If you want to enjoy a more recent\n');
fprintf('Psychtoolbox of version 3.0.10 or later, the automatic updater\n');
fprintf('will not do this for you.\n\n');
fprintf('Instead you need to run the DownloadPsychtoolbox() function.\n');
fprintf('This will *delete* your old Psychtoolbox folder completely and then\n');
fprintf('download a fresh copy of 3.0.10 or later from our new hoster GitHub.\n\n');
fprintf('After that download, you can use UpdatePsychtoolbox as usual to stay up to date,\n');
fprintf('or use the DownloadLegacyPsychtoolbox() function to downgrade back to an old\n');
fprintf('unsupported 3.0.9 or earlier legacy Psychtoolbox if you wish.\n');
fprintf('\n\n');
fprintf('Why would you want to upgrade to 3.0.10+? Because it provides you with\n');
fprintf('new features, bug fixes and improvements. Also because it will provide 64-Bit\n');
fprintf('support for MacOSX and (sooner or later) for Microsoft Windows.\n\n');
fprintf('Why would you want to stay away from it and stick to old versions?\n');
fprintf('Because the new 3.0.10+ versions will no longer support Apple PowerPC\n');
fprintf('computers running OSX and they will no longer support Matlab versions older\n');
fprintf('than V7.4, aka R2007a. Various other legacy features will also get removed.\n');
fprintf('\n');
fprintf('My advice: When in doubt, do the right thing.\n\n');
fprintf('Press any key to confirm that you read and understood this message.\n\n');

pause;

fprintf('\n\n\n');

return;

fprintf('Are we hosted on Google''s GoogleCode service, as we should be? Checking...  ');

% Check if this Psychtoolbox is still sourced from our old and (soon to be
% dead) hoster Berlios. Switch the working copy to source from Google-Code
% instead, if so:
checksvncommand = [GetSubversionPath 'svn info ' PsychtoolboxRoot];
if exist('system') %#ok<EXIST>
    [a,b] = system(checksvncommand);
else
    [a,b] = dos(checksvncommand);
end

% Assume no switch needed:
doSwitch = 0;
if (a ~= 0) | isempty(b) %#ok<OR2>
    % On failure to detect source URL, assume switch is not needed. This is
    % a tradeoff. Without it, we could fail miserably when called from
    % SetupPsychtoolbox on a system where the working copy is stripped of
    % .svn meta-info or the svn client is not installed:
    fprintf('Don''t know, don''t care.\n');
    return;
else
    if ~isempty(findstr(b, 'berlios'))
        % Still sourced from Berlios -> Switch needed.
        doSwitch = 1;
    end
end

% Switch needed?
if doSwitch
    % Yes: Rewrite SVN repository root URL to use GoogleCode from now on:

    % Tell user:
    fprintf('No :-( \n\n\n\n\n\n');
    fprintf('========================================================================\n');
    fprintf('Your current working copy of the Psychtoolbox is connected to our\n');
    fprintf('old Subversion repository at "https://svn.berlios.de". Berlios will\n');
    fprintf('shut down its services at the end of the year 2011. For this reason\n');
    fprintf('Psychtoolbox has moved to Google''s GoogleCode code hosting service.\n');
    fprintf('\n');
    fprintf('I will now switch from the old Subversion repository at Berlios to\n');
    fprintf('the new repository at GoogleCode: "http://code.google.com/p/psychtoolbox-3"\n');
    fprintf('\n');

    % Yes: Execute svn switch command:
    if ~isempty(findstr(b, 'svn://'))
        switchsvncommand = [GetSubversionPath 'svn switch --relocate svn://svn.berlios.de/osxptb http://psychtoolbox-3.googlecode.com/svn ' PsychtoolboxRoot];
    else
        if ~isempty(findstr(b, 'https://'))
            switchsvncommand = [GetSubversionPath 'svn switch --relocate https://svn.berlios.de/svnroot/repos/osxptb http://psychtoolbox-3.googlecode.com/svn ' PsychtoolboxRoot];
        else
            switchsvncommand = [GetSubversionPath 'svn switch --relocate http://svn.berlios.de/svnroot/repos/osxptb http://psychtoolbox-3.googlecode.com/svn ' PsychtoolboxRoot];
        end
    end
    fprintf('Executing: %s\n\n', switchsvncommand);
    fprintf('This may take a moment, please standby...\n\n');
    drawnow;

    if exist('system') %#ok<EXIST>
        [rc, msg] = system(switchsvncommand);
    else
        [rc, msg] = dos(switchsvncommand);
    end

    if rc ~= 0
        % Ohoh!
        fprintf('FAILED! Sorry, could not switch your working copy to new home for some reason.\n');
        fprintf('The Subversion error message was: "%s".\n', msg);
        fprintf('\n');
        fprintf('If the error message makes sense to you, you can try to fix\n');
        fprintf('the problem and then retry the Psychtoolbox update.\n');
        fprintf('\n');
        fprintf('You could also search the Psychtoolbox Wiki or forum if any\n');
        fprintf('troubleshooting tips for this problem are available. This Wiki\n');
        fprintf('page "http://psychtoolbox.org/MigrationToGoogleCode" is a good starting point.\n');
        fprintf('\n');
        fprintf('A reliable way to solve the problem is to start over from scratch:\n\n');
        fprintf('1. Manually delete the folder with your current copy of Psychtoolbox from\n');
        fprintf('   your computer. Also remove all entries of Psychtoolbox from your Matlab\n');
        fprintf('   or Octave path manually. Make sure to backup any files you may have modified\n');
        fprintf('   in your old copy of Psychtoolbox before you delete the folder!\n');
        fprintf('\n');
        fprintf('2. Get a *new* copy of the DownloadPsychtoolbox.m downloader script from the\n');
        fprintf('   Download-Section of our Wiki at http://psychtoolbox.org/PsychtoolboxDownload\n');
        fprintf('\n');
        fprintf('3. Use that new DownloadPsychtoolbox script to download a fresh copy of Psychtoolbox\n');
        fprintf('   from Google.\n');
        fprintf('\n');
        fprintf('Good luck.\n\n');

        error('Switch of repository failed. Please follow the troubleshooting instructions above.');
    end

    % Success: Tell user what needs to be done now:
    fprintf('Success! Psychtoolbox has been switched to use its new home at Google.\n');
    fprintf('\n');
    fprintf('To complete the transition and to make sure you really have the latest\n');
    fprintf('version of Psychtoolbox installed, please do the following:\n');
    fprintf('\n');
    fprintf('1. Restart Octave or Matlab as you wish. I will exit it for you in a moment.\n');
    fprintf('2. Run the download or install command you used (DownloadPsychtoolbox, or UpdatePsychtoolbox) again,\n');
    fprintf('   so i can complete the transition.\n\n');
    fprintf('\n');
    fprintf('Press any key on the keyboard once you read these instructions carefully. I will quit Matlab or Octave then.\n');

    pause;

    fprintf('Ok, please follow those two steps now. Thanks!\n\n');
    pause(4);

    % And... ...we're gone:
    quit;
end

fprintf('Yes :-)\n');
return;

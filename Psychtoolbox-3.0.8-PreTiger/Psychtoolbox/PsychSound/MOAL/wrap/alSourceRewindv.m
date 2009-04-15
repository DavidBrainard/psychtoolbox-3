function alSourceRewindv( ns, sids )

% alSourceRewindv  Interface to OpenAL function alSourceRewindv
%
% usage:  alSourceRewindv( ns, sids )
%
% C function:  void alSourceRewindv(ALsizei ns, const ALuint* sids)

% 06-Feb-2007 -- created (generated automatically from header files)

if nargin~=2,
    error('invalid number of arguments');
end

moalcore( 'alSourceRewindv', ns, uint32(sids) );

return

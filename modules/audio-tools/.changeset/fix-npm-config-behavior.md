---
"@oletizi/audiotools": patch
"@oletizi/audiotools-config": patch
"@oletizi/sampler-lib": patch
"@oletizi/sampler-devices": patch
"@oletizi/sampler-midi": patch
"@oletizi/sampler-translate": patch
"@oletizi/sampler-export": patch
"@oletizi/sampler-backup": patch
"@oletizi/lib-runtime": patch
"@oletizi/lib-device-uuid": patch
"@oletizi/sampler-attic": patch
---

BREAKING CHANGE: Installer no longer auto-modifies npm config

The installer will now fail gracefully with instructions instead of permanently changing your npm prefix without consent.

Lucas FM 0.3 evidence for source 0358b64dc0cb663632e6f008532ea14e0e1fd995.

*.metrics.json contains actual compiled-firmware measurements. Standard stress cases include scripts and hashes. maximum-7-input.json records the heavy test input. audio-checks.txt checks the play-ready package without an external flash image or sample overlay. The fresh strict package cases record one startup late counter, with no further late counters in tested playback. matching-checks.txt contains the two final matching-navigation regressions also included in tools/lucas_v03_check.py.

emulator-summary.txt records twelve screen destinations, recording/save/reboot/play and clock stress. voice7-test.txt checks retained heavy voices with visible CPU protection. usb-transfer.txt checks every transferred sample/header/metadata byte through compiled firmware. resources.json reports exact linked sizes. lucas-host.txt includes sanitizer-backed local checks and ffmpeg preparation tests. site-tests.txt checks offline asset packaging and generation with the exact FM-1_803 update package.

upstream.txt and lucas-ci.txt are the unedited final Linux CI outputs. Four static instruction-budget checks remain over budget; no golden or budget baseline was changed. The Chromium offline-site probe is skipped because Playwright is absent; the actual firmware browser boot was checked separately through Chrome.

The firmware build source differs from the later delivery commit only in documentation, artifacts and final regression-script additions. No private audio or private file inventory is included. Hardware remains untested.

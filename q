[33mcommit 7a316511db372cdd65e1187f93fe2e72e8b91768[m[33m ([m[1;36mHEAD[m[33m)[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Wed Mar 22 09:46:46 2023 +0100

    resolved 0x2005/0x2006 latch problem + scrolling begins to work

[33mcommit 8d7e5a997d1a0a8a388e038d42363194f94f91eb[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Sat Mar 18 23:32:27 2023 +0100

    sprite are showed with correct palettes. Still missing:
    scrolling
    sprite hit
    sprite overflow
    8x16px sprite mode implementation
    sprite priority.

[33mcommit ba898099342af323f43ecadd5c41dc4702f97da4[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Sat Mar 18 00:31:42 2023 +0100

    removed files

[33mcommit d7bd3e13d6bd52235c330513395adcd586fa8f77[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Sat Mar 18 00:30:56 2023 +0100

    update readme.md

[33mcommit 1bd426736558c3af449228123e28f1bc526597e6[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Sat Mar 18 00:01:38 2023 +0100

    still no scrolling, but great advancement was made to PPU (fixed 1-byte delay when reading CPU@0x2007,
    fixed sprites masking (CPU@0x2001))

[33mcommit 084207c83466aba09823527b8868bbe18dbec12e[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Wed Mar 15 01:34:43 2023 +0100

    ppu works much better, nametable is about to get fixed

[33mcommit 038c3870528c243a691f40ac5141ff3b4b82572a[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Tue Mar 14 02:28:57 2023 +0100

    updates, again

[33mcommit 83ebbe5ba4ed75308063a52275cf1a89a554494a[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Sat Mar 11 18:42:13 2023 +0100

    CPU is complete for legal operations (cycles)

[33mcommit 296091808dd65ccc941aba9d83facab00a3213ee[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Sat Mar 11 15:11:20 2023 +0100

    much update

[33mcommit fb977ca3c5a73798c2729e12f434b867210635e4[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Tue Mar 7 19:23:04 2023 +0100

    great advancement

[33mcommit 89fdfdd303ff93ceb42898f6cd758c18739a07ca[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Fri Mar 3 13:51:25 2023 +0100

    multi-system comp

[33mcommit b28baa67bbf08c894167ed0fad6d0dbfd94a01bd[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Thu Mar 2 15:20:49 2023 +0100

    minor refactoring

[33mcommit ccf578071cb09c099953433ab561e475d6d5273a[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Thu Mar 2 15:13:05 2023 +0100

    major cpu improvements

[33mcommit 0e657425f15dc79eb989337f49ae4facec21e5a0[m
Merge: a250f9e d956c16
Author: thenewservant <ppd491@protonmail.com>
Date:   Wed Mar 1 01:21:15 2023 +0100

    Merge branch 'main' of github.com:thenewservant/yanemu

[33mcommit a250f9ec720a806dc92c5a12fb410b9e8a28dd47[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Wed Mar 1 01:19:41 2023 +0100

    finally fixed SBC arithmetic

[33mcommit 35258ae3152210a7e9102e0ca3550cfff95f2d7e[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Sun Feb 26 23:40:00 2023 +0100

    lots of corrections, NESTEST.NES runs further

[33mcommit d956c16c0dfe0a4c297e66c39f08c78dd8b93a14[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Sun Feb 26 21:03:24 2023 +0100

    Create README.md

[33mcommit b8eb646db3174e291f1e67f53ecde5bc61a5fbb1[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Sun Feb 26 21:02:45 2023 +0100

    _

[33mcommit bb91467da512113d1cb8ce6ef078d0c29e903416[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Sun Feb 26 21:02:01 2023 +0100

    deleted useless files

[33mcommit ce3b4c984bd21fc506ccb2ab5bbab14bab5a9310[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Sun Feb 26 21:01:15 2023 +0100

    gitignore

[33mcommit 751e29d4e1b05566efd108c85992fd1f5bd771ad[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Sun Feb 26 20:59:43 2023 +0100

    tests in progress

[33mcommit 5403fd2d38d5f43e424ab535d5f30d442bcb02a7[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Wed Feb 22 10:24:32 2023 +0100

    SBC

[33mcommit 47b633df4ca06962e6f3929dce5e1b82a489099c[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Tue Feb 21 00:02:01 2023 +0100

    added support for V flag

[33mcommit 2ce74a4d18689bfc9dd2a067f00c1ab74554788e[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Sun Feb 19 22:59:58 2023 +0100

    improvements

[33mcommit d916e706aaa844baa32b690ababbdf0b1ee826fa[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Sat Feb 18 22:52:45 2023 +0100

    sound, other stuff

[33mcommit 1cea71aa617fd12a4170a7e341e4060d8c9ed99d[m
Merge: 7baa054 35f7045
Author: thenewservant <ppd491@protonmail.com>
Date:   Fri Feb 17 10:51:33 2023 +0100

    Merge branch 'main' of github.com:thenewservant/yanemu

[33mcommit 7baa05491f0289d5eff1e8313ebb99909a3e8bfd[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Fri Feb 17 10:51:16 2023 +0100

    commit

[33mcommit c95a9a790b8331094c6796ffb4d82fa21b86a443[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Fri Feb 17 10:49:45 2023 +0100

    added APU.cpp APU.

[33mcommit 35f704527c6efffa86586b64de6fff9041788711[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Fri Feb 17 00:31:26 2023 +0100

    Delete simpleCPU.h

[33mcommit 29c3e3f6f831b3f66438a189e59001feffeb8b56[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Fri Feb 17 00:29:39 2023 +0100

    first rel

[33mcommit 5058d10f7213e2dec010d4c497574944bd8565bd[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Thu Feb 16 23:04:52 2023 +0100

    added SimpleCPU

[33mcommit bfeab771f181122d31b3f5633fd1bdd0efd48444[m
Author: thenewservant <ppd491@protonmail.com>
Date:   Thu Feb 16 22:49:47 2023 +0100

    Initial commit

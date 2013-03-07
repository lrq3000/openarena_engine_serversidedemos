Unofficial port of OpenArena 0.8.8 client/server to the latest ioquake3 on git (rebased)
========================================================================================

This is based off of r28 of the binary thread which was used in the release
client/server for OpenArena 0.8.8.

It is intended to be as close as possible to 0.8.8 except when it makes
sense to deviate.

OpenArena 0.8.8 uses r1910 ioquake3 code.  This code currently targets
the ioq3 github repo, which is above r2398 (latest rev on SVN).

This port was originally made by Undeadzy (https://github.com/undeadzy/openarena_engine), so all credits go to him.

This is a CLEAN PATCH: it was redone manually from scratch, thus it contains only the minimal subset of changes that OA 0.8.8 applied to ioquake3, while keeping the maximum of ioquake3 original code (and when I say a maximum, I mean it - just check the diff to see for yourself).

This repo is different from Undeadzy's original repo: while Undeadzy chose to fetch+merge the changes (thus interleaving and keeping the commits in their original chronology, but mixing between ioquake3 commits and openarena_engine), here the choice was to fetch+rebase the commits, so that you get the ioquake3 commits all in their space, and then on top you get the openarena_engine commits. I think this linear graph is better for our purpose since it should help maintaining future versions of this engine and keep track of the changes from the ioquake3 engine (which BTW merged a lot of changes from openarena in the past months).

Also, I explain below how to update this repo with the latest ioquake3 repo on github.

Switching renderers
-------------------

Recent ioquake3 versions allow for a modular renderer.  This allows you to
select the renderer at runtime rather than compiling in one into the binary.

To enable this feature, make sure Makefile.local has USE_RENDERER_DLOPEN=1.

When you start OpenArena, you can pass the name of the dynamic library to
load.  ioquake3 assumes a naming convention renderer_*_.

Example:

    # Enable the default ioquake3 renderer.
    $ ./openarena.i386 +set cl_renderer opengl1

    # Enable the OpenArena renderer with GLSL, bloom support and more.
    $ ./openarena.i386 +set cl_renderer openarena1

Building
--------

This a standard ioquake3 build which they describe here:

http://wiki.ioquake3.org/Building_ioquake3

It's not an autotools based build.  If you don't have the dependencies, it
will break in the middle of the build.

If you are on Ubuntu or Debian, the easiest way to compile this is to install
the build dependencies for the "ioquake3" package.

    $ sudo aptitude build-dep ioquake3
    $ git clone git://github.com/lrq3000/openarena_engine.git
    $ cd openarena_engine
    $ make

You may want to change a few things in Makefile.local.  Other than installing
the build dependencies, you shouldn't need to do anything else.  By default it
builds a modular renderer so you need the binary + *.so or *.dll files in the
same directory to run it.

Development
-----------

    # Get this project
    $ git clone git://github.com/lrq3000/openarena_engine.git
    $ cd openarena_engine

    # Create a reference to the upstream project
    $ git remote add upstream git://github.com/ioquake/ioq3.git

    # View changes in this project compared to ioquake3's github
    $ git fetch upstream
    $ git diff upstream/master

UPDATE ON GIT
-------------

This repository was rebased onto the latest ioquake3 repo on github, so that all changes from ioquake3 to openarena are on top of the latest ioquake3 changes. Thus, you have a linear commit graph which is easy to rebase against new versions, and the history is more human readable, because you can clearly separate the ioquake3 changes from the openarena_engine changes.

To update this repo and sync with the latest ioquake3 revision, you can:

    # Get this project
    $ git clone git://github.com/lrq3000/openarena_engine.git
    $ cd openarena_engine

    # Create a reference to the upstream project
    $ git remote add upstream git://github.com/ioquake/ioq3.git

    # Optional: for your own information, you can view changes in this project compared to ioquake3's github
    $ git fetch upstream
    $ git diff upstream/master

    AUTO-PROCEDURE
    ##############
    # To sync, try to pull + rebase the changes automatically
    $ git pull --rebase upstream master

    # If it works, then all is done! You have an updated version on your computer. You can stop here and just compile the engine using the tutorials in the ioq3 wiki: http://wiki.ioquake3.org/Building_ioquake3
    # PS: for Windows, use the cygwin tutorial, don't try mingw unless you are very experienced and have got a lot of spare time to mess with the weird errors you will get.

    MANUAL PROCEDURE
    ################
    # Else, you have to redo the process but manually this time. Following steps:
    
    # First, you have to reinitialize you repo, else it won't accept you do anything:
    $ git rebase --abort
    $ git reset --hard origin/master

    # Git rebase interactive so that you get the list of the last commits. This will open you a text file in your default text editor program. What you have to do here is to cut all the lines that concerns commits about the openarena_engine patch. Then paste these lines in a temporary text file for you (we will later use the SHA code of the commmits).
    $ git rebase HEAD~10 --interactive

    # Now you should have a clean repo without the openarena_engine commits, only an old revision of the ioquake3 engine.

    # We will now sync to the latest changes of openarena_engine or ioquake3:
    $ git pull -s recursive -X theirs upstream master
    # This will pull all remote changes and accept them automatically.

    # Now, you should have a clean repo synced to the latest revision of the engine. We just have now to reapply the openarena_engine commits one by one, on top of the latest ioquake3 engine.

    # Reopen the text file where you pasted those openarena_engine commits lines before (when we did rebase --interactive), and follow the commits in the natural order: from the top to the bottom. The top commit being the oldest one, and the lowest the latest.
    # You have to pick the oldest commit first, then to the more recent and so on. Look at the SHA code (the second column after the command "pick"), and copy this SHA code below in place of 617a:
    $ git cherry-pick 617a
    # This command will pick this very specific commit and will try to reapply it.
    # It will try to automatically merge the conflicts.

    # In the case there are some unresolved conflicts, you can resolve them manually by doing:
    $ git mergetool

    # If you resolved manually the conflicts, then you have to do:
    $ git commit

    # And then, you can continue onto the next openarena_engine commit:
    $ git cherry-pick ...

    # Until you reapplied all the commits, and voila, you have an up-to-date engine with the openarena_engine patch!

Changes from 0.8.8 release
--------------------------

* OA's renderer is now in renderer_oa and the base ioquake3 renderer is left
  untouched
* This does not remove the game or cgame files.  They are never referenced or
  built.  This makes it easier to keep in sync with upstream.  It would also
  be possible to integrate the OA engine and OA game code at some point in the
  future.
* Makefile has fewer changes since the recent upstream Makefile makes it easier
  to support standalone games
* cl_yawspeed and cl_pitchspeed are CVAR_CHEAT instead of removing the
  variables and using a constant.
* r_aviMotionJpegQuality was left untouched
* Enables STANDALONE so the CD key and authorize server related changes are
  no longer needed.
* Enables LEGACY_PROTOCOL and sets the version to 71.
* This uses a different GAMENAME_FOR_MASTER than 0.8.8.  It also uses the new
  HEARTBEAT_FOR_MASTER name since the code says to leave it unless you have a
  good reason.
* Any trivial whitespace changes were left out
* Added James Canete's Rend2 renderer (v31a+) as a branch named
  opengl2_renderer
* GrosBedo added win32 support back to the Makefile

TODO
----

* Try to avoid changing qcommon area to support GLSL.  Canete's opengl2
  didn't need this change so this renderer shouldn't either.
* Remove compiler warnings.  I kept them in for now so the code would be
  as close as possible to 0.8.8.
* Verify that allowing say/say_team to bypass Cmd_Args_Sanitize is safe.
* Build in FreeBSD
* Build in MacOSX
* Cross compile in Linux for Windows
  - Cross compiling for Windows may require more changes to the Makefile to
    enable ogg vorbis support
* vm_powerpc_asm.c includes q_shared.h but upstream doesn't.  Why is this
  change in 0.8.8?
* Needs more testing
* Verify changes with OpenArena developers
* Potential GLSL debugging fix that was made available after 0.8.8 release.

Original file
-------------

OpenArena 0.8.8 client/server release source code:

http://files.poulsander.com/~poul19/public_files/oa/dev088/openarena-engine-source-0.8.8.tar.bz2

    $ sha1sum openarena-engine-source-0.8.8.tar.bz2
    64f333c290b15b6b0e3819dc120b3eca2653340e  openarena-engine-source-0.8.8.tar.bz2


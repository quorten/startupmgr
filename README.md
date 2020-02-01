`startupmgr`
============

`startupmgr` is a nearly complete but-not-quite startup process and
service manager for Windows XP.

Why?
====

Back in the heyday of Windows XP, from the years 2002 until 2012, it
was quite common for random software downloaded from the Internet to
register a startup task.  The most iconic such startup task was the
"Java Quick Starter" or `jqs.exe`.  The name of the process implies
that it will speed up your system, but in practice, it served mainly
to slow down most people's systems.  Why?  Well, at the time, hard
disk speeds were very slow due to the use of spinning magnetic hard
disk drives, so any time software would need to be loaded, there would
be a considerable performance penalty on your computer system, making
it unbearably slow during the startup time of software when the most
intensive hard disk loading was happening.  Not to mention that the
slow RAM speeds didn't help this either.  Now, this might have been a
price worth paying were you to use lots of Java software, but the most
common case was that people never used Java software within the time
frame of one PC boot and power-off.

`jqs.exe` wasn't the only culprit, but indeed it is a good
representative of the reasoning behind the dozens of other startup
tasks that piled up at boot-time to slow down your PC's boot.
Initially, I successfully combated the beast on one PC by simply
disabling absolutely all unneeded startup processes and services, and
I only manually started them when I really needed them.  Alas, I
wanted to expand this optimization to another computer that was more
hands-free from me, and I knew the manual procedures wouldn't fly
there.  So, I started writing "startup manager" (`startupmgr`), a
program that would _quickly_ start at boot and then present an
easy-to-use wizard to guide the user along whether they would want to
load all startup processes and services or just the bare minimal.

Why didn't I finish `startupmgr`?  Well, as I was hinting, I was
writing it deliberately to target a computer that I didn't very often,
and over the course of developing the software, well I figure that I
can bear the slowdown for the few times I needed to use the other
computer.  Then the hardware got decommissioned, Windows XP became
obsolete, and that was the end of it.

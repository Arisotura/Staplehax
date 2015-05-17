Staplehax
=======

Staplehax is essentially a mix of Ninjhax and libkhax.


The purpose is to get rid of some limitations of the original Ninjhax:
 * no CSND access on New 3DS
 * broken APT functionality
 

The first stage of Ninjhax is used to gain code execution under Cubic Ninja's process. Then libkhax is used to gain kernel access, launch homebrew as a separate process, and terminate CN.

This sounds like playing Lego with code, but in reality it's a lot harder than that.


Staplehax is compatible with firmware versions up to 9.2 (for more details, see what libkhax can run on).


### Current status

Very alpha. Will probably not be useful to you under this state.


### Building Staplehax

See: how to build Ninjhax. Just scrap the parts about the spider/skater oss.cro, Staplehax doesn't need those. You can also build it with somewhat-recent ctrulib.


You are on your own. Don't bother me.


### Credits

 - smea — original Ninjhax
 - Myria — the neat libkhax
 - various #3dsdev people who have helped me

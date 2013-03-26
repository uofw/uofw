uOFW
====

The unofficial Official FirmWare, a complete latest PSP firmware reverse engineering project.  
 
Doxygen documentation: http://uofw.github.com/uofw/   
Module status list: https://github.com/uofw/uofw/wiki/Current-Modules-Status
For more information: https://github.com/uofw/uofw/wiki  


## Getting started

In order to contribute to uOFW, you need advanced knowledge in MIPS ASM and C. Some good MIPS ASM  
online resources can be found [on the official MIPS page](http://www.mips.com/products/product-materials/processor/mips-architecture/).  
MIPS ASM Instruction Set Reference: http://math-atlas.sourceforge.net/devel/assembly/mips-iv.pdf  

In addition, there is a nice MIPS runtime simulator and debugger which can be found at:  
http://courses.missouristate.edu/kenvollmar/mars/

The next step is to improve your reverse engineering skills (if needed). We suggest reading [this  excellent guide]
(https://github.com/uofw/uofw/wiki/Reverse-engineering-tutorial) about reverse engineering for the PSP. 

Make sure to check out [uOFW's wiki](https://github.com/uofw/uofw/wiki) for more information!


## Installation & Running

For information on how to install and run uOFW, please look [here](https://github.com/uofw/uofwinst).


## Repository structure

uOFW's Github repository contains one 'master' branch and several sub-branches. 

The master branch is aimed to contain only tested and working code (in fact, it should contain working modules). 
Please note that this is not entirely the case right now as we still need to test, and fix, some of the code in the 
master branch due to the past repository structure.

Then there are sub-branches which are aimed to each represent a module currently being worked on. Such a branch
contains unfinished, untested and undocumented code about the module it is named after. Once such a branch contains
finished, documented code (that is, the module has been finished reverse-engineering) it will be merged into
the 'master' branch.
Currently, there are five sub-branches: [clockgen] (https://github.com/uofw/uofw/tree/clockgen), 
[libaac](https://github.com/uofw/uofw/tree/libaac), [memlmd](https://github.com/uofw/uofw/tree/memlmd), 
[power](https://github.com/uofw/uofw/tree/power), [psheet](https://github.com/uofw/uofw/tree/psheet)


## Additional Information

While you reverse-engineer parts of the PSP's kernel, you may will find yourself in a position
where you would like to know more about how Operating Systems work in general. We suggest to check
out [this webpage](http://wiki.osdev.org/Expanded_Main_Page).

If you are looking for more PSP-related resources, you should consider checking out the following links
below:

* [LAN.st - IdStorage](http://lan.st/archive/index.php/t-151.html)
* [LAN.st - General PSP questions](http://lan.st/archive/index.php/t-3013.html)
* [LAN.st - PSP Motherboard information](http://lan.st/archive/index.php/t-372.html)

* [Yet Another Playstation Portable Documentation](http://hitmen.c02.at/files/yapspd/psp_doc.pdf.tar.gz)
* [PSP Module Tutorial](http://pspdev1.com/wp-content/uploads/2007/03/moduletutorialv1.pdf)
* [24C3 - TyRaNiD on early PSP hacking + Pandorra hack](https://www.youtube.com/watch?v=INdUZk4NFIA)

Other websites are: 
* http://wololo.net
* http://ps2dev.org


## Questions or need help?

You can stay in touch with us and discuss project-related topics with us via the following ways:
* via IRC on the [FreeNode server](http://webchat.freenode.net/) (irc.freenode.net) (join the channel #uofw)
* via e-mail


## Copyright and License

For copyright and license information, please check [this file](https://github.com/uofw/uofw/blob/master/COPYING).

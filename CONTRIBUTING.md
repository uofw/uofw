#uOFW PSP kernel coding style

This document outlines uOFW's workflow and preferred code style. Please make yourself familiar with this document before you begin working.

## Workflow
We tend to be generous in giving collaborator status which gives you direct push access to uOFW. To keep some degree of stability in the repo, please follow these guidlines:

* Use topic branches
* Get reviews before merging pull requests into master
* Merge your branch to master only when it is stable
* Delete your branch after merging

A few do-nots:
* Do not push anything that could break a build directly to master (use a PR)
* Do not rewrite public branch history (e.g. don't rebase a branch you've already pushed)
* Do not merge/cherry pick across branches
* Avoid merging/cherry picking from master

All of these rules are meant to keep history clean and to avoid stepping on other developers toes. If you are unsure, do not hesitate to ask us on IRC.

## Coding style

Please stick to the style presented in this guide to make working on uOFW as a team easier as far as possible.
We know that coding style is very personal and we don't say you have to completely
follow it, however, your own used coding style for uOFW should not differ too much
from this one.
Thank you.


###Chapter 1: Indentation

Indentations are 4 bytes wide. The reason behind this is to visibly show
where a block starts and where it ends.

Example:

    if (condition) {
        doX();
        doY();
    }

Don't put multiple statements on a single line:

    /* bad */
    if (condition) doX();

For switch blocks, don't indent the `case`s and `default`s but only their content.
For example:

    switch (type) {
    case 'A':
        break;
    }


###Chapter 2: Breaking long lines

Make sure to not write code lines longer than 120 characters. If necessary, 
break lines exceeding the 120 characters limit like below:

    SCE_MODULE_INFO("sceController_Service", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD 
                                             | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 1);

This applies to functions as well.

Long `if`-lines (same as `while`-, `for`-lines) are broken into several lines like this:

    if (a == 1 ||
      b == 2)
        doSomething();


###Chapter 3: Placing Braces and Spaces

Put the opening brace last on the line, and put the closing brace first, thusly:

    if (condition) {
        doX();
        doY();
    }

This applies to all non-function code blocks (`if`, `switch`, `for`, `while`). 
Furthermore, it is used when using `typedef` or array/structure initialization.

However, don't put braces where only one branch exists with just one single 
statement.
Reason: The indentation is already visible and thus there is no need for braces.

    if (condition)
        doX();

If there are two or more branches, use braces this way:

    if (condition) {
        doX();
        doZ();
    } else if (otherCondition) {
        doA();
        doB();
    } else {
        doY();
        doW();
    }

And don't put braces when the block contents are only one line long, the same way
as above:

    if (condition) {
        doX();
        doZ();
    } else if (otherCondition)
        doY();
    else
        doW();

For functions, put the opening brace first on a new line and the closing brace 
first on the last line:

    void function(void)
    {
        doSomethingCool();
    }

####Space policy:

Put a space after these keywords: `if`, `switch`, `case`, `do`, `while`, `for`, `return`.

    switch (number) {
    case 1:
        doThis();
        break;
    case 2:
        doThat();
        break;
    default:
        doStandard();
        break;
    }

Don't put spaces after keywords not mentioned above.

Put spaces before and after arithmetic operators (`+`, `-`, `*`, `/`, `%`), shift operators, 
binary operators (`&`, `|`, `^`), comparison & operation operators.

Example:

    if ((a + (b << 2)) > 4)
        doSomeMagic(); 

Consequently, don't put spaces before and after prefix/postfix operators (`++`/`--`),
or after unary operators like `&`, `*`, `!`, `~`.
Furthermore, no spaces around structure member operators (`.` and `->`).

The `sizeof` compile-time unary operator is special. Use a space after it only when
taking the size of an object, don't use a space (but parentheses!) when taking
the size of a data type.

    sizeof object; -> (sizeof variable;)

    sizeof(type name); -> (sizeof(u32);)

Declaring a pointer is made by adjoining the unary operator `*` to the data name
and not the type name:

    s8 *ptr;

When declaring a pointer to a function, this way suffices:

    s32 (*funcPtr)(void);


###Chapter 4: Type using

Don't use type's like `(unsigned) short`, `(unsigned) int` as their size is NOT
the same on all machines.
Use instead:

    8 bit signed/unsigned data size -> s8/u8
    16 bit signed/unsigned data size -> s16/u16
    32 bit signed/unsigned data size -> s32/u32
    64 bit signed/unsigned data size -> s64/u64

The `char` type is a bit special: it is different from both `s8` and `u8`. That way,
any string between quotes will not be a `s8*` or a `u8*`, and you would have to use
an ugly cast.
In that case and cases where the data clearly contains a printable character,
use the `char` type. In all other cases, use either `s8` or `u8`.

Also use these data types for casts, when necessary.

Use the appropriate type for a variable, i.e. if you have a boolean variable, 
use `SceBool` and not `u32`. Another example: Use `SceSize` for variables holding
sizes of a file, a memory block, etc. You can find a list of possible types in
`/include/common/types.h`.

When declaring a structure type using a `typedef`, do it this way:

    typedef struct {
        s32 x;
        s32 y;
    } Point;

and not:

    /* bad */
    typedef struct _Point {
    s32 x;
    s32 y;
    } Point;

Except if defining the structure itself is needed, in example if the structure 
references itself. In that case, use:

    typedef struct Point {
        struct Point *next;
    } Point;


###Chapter 5: Naming

Use simple, short names, easy to understand. For local variables, it is even okay
to use a variable name consisting of only one letter:

    /* bad */
    u8 loopCounter;

    /* good */
    u8 i;

Another common example is the use of temporary variables. Just name them `tmp`,
this is clearly understandable and does its job. Don't try making the program
harder to understand than necessary.

If you encounter a local variable and you don't know what it represents, name 
it `unk`. If there are more unknown variables within one function, name them
like this:

    s32 unk1;
    s32 unk2;
    ...

This applies for unknown structure members too. Note that if there are a few
different structures in the same file, using different "styles" (like `unk1` or
`unk001` or `unk_1`...) can be useful so the member name can be replaced easily in all
the file.

For global variables, use descriptive names. Here, the variables' names
matter and not primarily the name length.
In order to clearly distinguish global variables from local variables
(especially helpful in large functions), use a `g_` prefix for global's.

    /* global mutex id */
    s32 g_MutexId;

Don't use underscores in variable names, except for the `g_` prefix for globals. 
Instead, use capital letters.

    /* Bad */
    SceBool is_uofw_cool;

    /* Good */
    SceBool isUofwCool = SCE_TRUE; /* Indeed. */


###Chapter 6: Magic numbers

Using magic numbers is bad, as only the author of the C file in which they occur
will know (for a little while) what they express.
Instead, use a `#define` statement or an enumeration to give numbers a meaningful name.

The approach is to give hardware important `#defines` a `PSP_` prefix and general 
`#defines` shared with other files a `SCE_` prefix, as in `SCE_MODULE_KERNEL`.

Hardware-important example `#define` from `ctrl.c`:

    /* This is bad, what SYSCON transmit command does 7 represent? */
    g_ctrl.sysPacket[0].tx[PSP_SYSCON_TX_CMD] = 7;

Thus, we have the following `#define` in `syscon.h`:
    #define PSP_SYSCON_CMD_GET_KERNEL_DIGITAL_KEY    7

    /* Aha, "7" stands for a data tranfer of all digital buttons (no analog pad data). */
    g_ctrl.sysPacket[0].tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_GET_KERNEL_DIGITAL_KEY;

The same goes for return codes. uOFW's error file is located in 
`/include/common/error.h.` Use the defined errors instead of magic numbers.

A similar approach for `for`/`while`-loops:

    /* This is bad code */
    u32 g_Array[10];

    u32 i;
    for (i = 0; i < 10; i++)
        g_Array[i] = 0;

    /* Good code */
    #define ARRAY_SIZE 10

    u32 g_Array[ARRAY_SIZE];

    u32 i;
    for (i = 0; i < ARRAY_SIZE; i++)
        g_Array[i] = 0;


###Chapter 7: Structure initialization

Assume we have the following structure:

    typedef struct {
        s32 x;
        s32 y;
    } Point;

The initialization of a Point variable looks like this:

    Point p = {
        .x = 20,
        .y = 12
    };

Directly assigning the structure members is very helpful, especially in large
structure initializations, as this will help us to keep track of which structure
member has been initialized with what value.

Use the `sizeof` operator when a structure has a `size` field to be filled with the
    structure size (sizeof(structureType);)


###Chapter 8: Accessing hardware registers

Hardware registers are stored in memory starting at `0xBC000000` and are _special_
compared to global variables. They have to be written synchronously, which means
you have to tell the compiler that it can't invert two hardware operations.

If you want to load from/store to a hardware register, use the `HW(addr)` macro. 
You can find this macro, and other useful `#define`s, in `/include/common/hardware.h`.

    u32 oldRamSize;

    oldRamSize = HW(HW_RAM_SIZE);
    HW(HW_RAM_SIZE) = RAM_TYPE_64_MB;


###Chapter 9: Comments

All exported functions of a module and important data structures as well as
`#defines` shared among `.c` files have to be put into a header file used as a module 
documentation.

Exported functions are commented like this:

    /**
     * Purpose of <exportedFunction>.
     * 
     * @param a Description of parameter a.
     * @param b Description of parameter b.
     * 
     * @return 0 on success.
     */
    u32 exportedFunction(u32 a, u32 b);

> Note: "0 on success." implies that the function returns less than zero on error.
> We do that because all the exported functions return less than zero on error. If
> the function always returns 0, just use `@return 0.`.

Structure members are commented like this (same for enumerations):

    typedef struct {
        /** The X-axis value of a point. */
        s32 x;
        /** The Y-axis value of a point. */
        s32 y;
    } Point;

In the module's `.c` file(s), use `/*` instead of `/**` and only add
parameter description if the purpose of the parameters is unclear without
additional description.

    /*
     * Purpose of <sum>.
     * 
     * Returns the sum of a and b.
     */
    static s32 sum(s32 a, s32 b);

Here, everything about the parameters a and b is obvious, so there is no need for 
explaining them.
Also note that we use `Returns...` instead of `@return` for header functions.

###Comments inside of functions:

When necessary, comment hard-to-understand code parts, explaining WHAT the code is doing.
The reader of the `.c` file probably knows a lot less than you (as you reversed the module) 
about the module. Thus, make sure that those readers will understand what is going on.

One line comments will look like this:

    /* My comment. */

Longer comments differ from short comments:

    /* 
     * My comment, which is too long
     * to fit in one line.
     */

Use `//` for temporary comments such as `//TODO: ` or address comments like 
    sceKernelCpuResumeIntr(intrState); //0x000020A8

Make sure to use enough address comments to be able to fix every line in in your
reversed code file fast and easily. Keep a local copy of your reversed code files
with included address comments and, optionally, TODO-comments, and upload a file version 
WITHOUT address-, TODO-comments to the uOFW repositiory, as soon as your module fully works.


###Chapter 10: Switches

Put a break at the end of each `case` and at the end of the `default`.
Don't skip lines between the different labels (`case`s and `default`s).
For example:

    #define UOFW_IS_COOL            1
    #define UOFW_IS_VERY_COOL       2

    switch (type) {
    case UOFW_IS_COOL:
        MakeUofwVeryCool();
        break;
    case UOFW_IS_VERY_COOL:
        MakeUofwSuperAwesome();
        break;
    default:
        SetUofwIsHappy();
        break;
    }


###Chapter 11: Export files

Only use the `PSP_EXPORT_NID` command, so a function or variable name can easily be found from
the NID using the `exports.exp` file, and to sometimes avoid mistakes with user functions.
If the NID is not randomized but doesn't correspond to the function name, specify the string
the NID comes from in a comment, the line before.
For example:

    # sceKernelDcacheInvalidateRange
    PSP_EXPORT_FUNC_NID(sceKernelDcacheInvalidateRangeForUser, 0xBFA98062)

Specify at the beginning of every library whether its NIDs are randomized or not.

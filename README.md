A 6502 emulator with an interface for stepping through NES ROMs.

Here's the emulator going through the first few instructions of Super Mario Bros:

[![asciicast](https://asciinema.org/a/15af771rh9ophtxhjl2k5x5cf.png)](https://asciinema.org/a/15af771rh9ophtxhjl2k5x5cf)

It falls into an infinite loop at the branching instruction at 0x800D for what seems to be IO-related reasons (the preceding instruction, LDA at 0x800A, loads the value at 0x2002 which is an IO register).

We can peek a bit further by replacing the branching instruction with a NOP (and do the same for next few branching instructions):

[![asciicast](https://asciinema.org/a/18kiirp0vjclj649i040u44kv.png)](https://asciinema.org/a/18kiirp0vjclj649i040u44kv)

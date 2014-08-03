checkpoint_utility
==================
How to compile?
_______________

Change to working directory and run this command: ``make``

It will create kernel Module(LKM).

Insert module
_____________

Run following commands:
``insmod checkpoint.ko``
``insmod restore.ko``

Using utility
_____________

Compile other C file using GCC.
Now create any running process you want to checkpoint(For test you can use *keeponrunning.c*) and note pid(PID) of that process.

Run following command
``gcc -o main main.c``
``./main PID``

it will create checkpoint file in your home Directory.

Similarly you can restore the process.

NOTE:
=====
**Utility is not restoring process at this moment. we are working on it**






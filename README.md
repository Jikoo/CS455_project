# PrivateNotes

Final project for CS-455 Principles of Secure Software Development.  
A basic C program for making private notes.

Compile with `gcc menu.c security.c data.c -lcrypto -Wall -o notes`.  
Certain operating systems may also require `-lssl` or `-lbsd` flags.

To run, execute `./notes` after compiling.  
Optionally, use `-p` to supply password, i.e. `./notes -p "This password is not very secure due to being published."`.

Execution flow:
```
Check for cli parameter for password
If secure store does not exist
  If password is not provided
    Intake password
  Create secure store
Else
  If password is not provided
    Intake password
  Authenticate

Prompt for user action
  View:
    List all notes and content
    - requires decryption of notes
  Add:
    Prompt for note content
    Encrypt and save to disk at next note ID
  Delete:
    Prompt for selection
    Delete from disk
    Move subsequent notes to fill gaps?
  Exit: Yep.
```

# PrivateNotes

Final project for CS-455 Principles of Secure Software Development.  
A basic C program for making private notes.

Execution flow:

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

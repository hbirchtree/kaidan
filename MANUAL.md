Here's the syntax of Kaidan-compatible files:

 - A "variables" array filled with objects:
  * variables may have the following keys:
    - name : The name of the variable
    - value : The value of the variable. May reference other variables.
  * You are provided with a static variable, %STARTDIR%, which is set to the parent directory of the JSON-file.
  * Environment variables, filenames, commandlines and directory names have substitution applied to them.
 - An "environment" array filled with objects:
  * Only supports variables.
  * Supported keys for objects:
    - type : can only be "variable"
    - name : the name of the environment variable
    - value : the value, is substituted
 - An object "kaidan-opts"
  * Contains miscellaneous options for Kaidan
  * Supported keys:
    - import-env-variables : which environment variables to import as internal variables
    - shell : the name/filename of the shell executable, used for executing commandlines
    - command.argument : the argument used in conjunction with the shell to accept a commandline
    - window-title : window title. what else would it be?
    - icon-size : the size of the icons in pixels, used for scaling them to this specific size. if this is not set, icons will expand the UI to their size. (Which is ugly) I recommend a size of 128 for normal displays.
 - A "steps" array filled with objects:
  * First element to be run has a key "init-step" with a boolean value of true.
  * Each element can have an icon and title, both of which shown in the interface.
    - desktop.title : the title
    - desktop.icon : the icon's path
  * Must have a "type" key of type string
  * Should have a "name" key of type string in order to refer to the step
  * Uses the key "proceed-to" of type string to refer to the next step in the proces. If it's the last step, this key is omitted.
  * The different types (strings given to the "type" key):
    - "install-payload" has the following keys:
      * source : the source file or directory that is copied
      * target : the target file or directory that is coped TO (Note: If you copy a file, it will not create parent directories. Use a directory payload for this.)
    - "download-file" has the following keys: 
      * source : a URL, only supports public URLs without authentication so far, but will typically support HTTP, HTTPS and FTP.
      * target : the target file, will not create any parent directories. You typically don't want it to download straight into the user's filesystem, keep it in the assets directory and use a payload step instead.
      * If download is unsuccessful, the step fails and will halt the whole process.
    - "execute-commandline" has the following keys:
      * workdir : the directory to execute the command in
      * commandline : the command line for the shel to process
      * lazy-exit-status : usually, the exit status of the process will be used to determine whether to proced or not. If this value is true, the exit value will always be ignored.
    - "execute-file" has the following keys:
      * Similar to "execute-commandline" but executes files without the shell. This may be useful in some cases.
      * workdir : the directory to execute the file in
      * target : the target file to execute, with path to the file since the shell is not being used.
      * validate : whether to validate if the execution process returns 0 or not.

      

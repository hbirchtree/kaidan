kaidan
======

The purpose of Kaidan will be a way of performing serial tasks like installations complete with validation and things like that.

##What is Kaidan capable of?
 - Installing a payload, which may be a file or directory filled with content.
 - Running a shell command. As long as it is written to be compatible with the system, it *might* work even with Windows. (Note: I don't care about Windows support.)
 - Executing a file with arguments. (Not thoroughly tested, arguments and QProcess are an odd thing. I might have to do some wonky yet small change for it to work correctly, or maybe not.)
 - Validation of exit codes and even ignoring exit codes entirely, all up to the user.
 - Downloading files from the internet. So far, it is not 100% implemented and it will only download files that are publicly available. It doesn't seem too hard to add support for authentication.
 - Based around having a folder filled with assets, but these assets may be pulled from the internet by the help of the JSON file. It is all focused around flexibility.

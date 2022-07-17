# Dependency-Manager
A simple dependency manager that pulls from git and copies individual files or directories. (other sources may be implemented later, such as http or svn)

See vendor.txt for example usage.

To use this depencency manager you must create a vendor.txt file inside of you project and populate the files with commands of the following type.

`git "{source url}" {source branch/tag/commit} "{relative dir from git project, what needs to be copied}" "{relative dir of current project, where it needs to be copied}",`

All commands must end with a comma at the end.

Installing the project is as simple as copying the executable `git-vendor` to the `/bin` or `/usr/bin` or `/usr/local/bin` directory. After installation you can simply cd into the current project dir with a vendor.txt file and run `git-vendor` to pull dependency files.

The original intended usage for this project is for C++ but it can work with any language.

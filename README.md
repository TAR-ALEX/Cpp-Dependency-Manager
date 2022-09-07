# Dependency-Manager
A simple dependency manager that pulls from git and copies individual files or directories. It can also download tar files and compressed tar files and extract certain directories or files from them and place them in desired locations (other sources may be implemented later, such as http or svn)

See vendor.txt for example usage.

To use this depencency manager you must create a vendor.txt file inside of you project and populate the files with commands of the following type.

For git:

`git "{source url}" {source branch/tag/commit} "{relative dir from git project, what needs to be copied}" "{relative dir of current project, where it needs to be copied}",`

For tar:

`tar "{source url}" "{relative dir from git project, what needs to be copied}" "{relative dir of current project, where it needs to be copied}",`

All commands must end with a comma at the end.

Installing the project is as simple as copying the executable `git-vendor` to the `/bin` or `/usr/bin` or `/usr/local/bin` directory. After installation you can simply cd into the current project dir with a vendor.txt file and run `git-vendor` to pull dependency files.

The original intended usage for this project is for C++ but it can work with any language.

libraries that are needed to run this are:

```
libssl.so.1.1 
libcrypto.so.1.1
liblzma.so.5 
libz.so.1
libbz2.so.1.0
```

```
sudo apt install build-essential libbz2-dev liblzma-dev libz-dev libssl-dev git
```

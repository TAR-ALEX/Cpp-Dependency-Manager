# Dependency-Manager
A simple dependency manager that pulls from internet repositories and copies individual files or directories. It will detect conflicting files between repositories. It can also download tar (.tar) files and compressed tar (tar.gz, tar.xz, tar.lzma) files and extract certain directories or files from them and place them in desired locations. Debian (.deb) packages can be installed locally along with their dependencies. Runnning this dependency manager does not install anything on the system globally, unless directed to copy to a global path (do not run with sudo)

See vendor.txt for example usage.

To use this depencency manager you must create a vendor.txt file inside of you project and populate the files with commands of the following type.

For git:

```
git "{source url}" {source branch/tag/commit} 
"{relative dir from git project, what needs to be copied}" 
"{relative dir of current project, where it needs to be copied}",
```

For tar:

```
tar "{source url}" 
"{relative dir from git project, what needs to be copied}" 
"{relative dir of current project, where it needs to be copied}",
```

Sample usage for deb:
```
(configure the debian sources)
deb-init [
    "deb http://packages.linuxmint.com una main upstream import backport",
    "deb http://archive.ubuntu.com/ubuntu focal main restricted universe multiverse",
    "deb http://archive.ubuntu.com/ubuntu focal-updates main restricted universe multiverse",
    "deb http://archive.ubuntu.com/ubuntu focal-backports main restricted universe multiverse",
    "deb http://security.ubuntu.com/ubuntu/ focal-security main restricted universe multiverse",
    "deb http://archive.canonical.com/ubuntu/ focal partner",
    "deb http://ftp.us.debian.org/debian buster main ",
],
(configure the recursion limit, install subdependencies only to depth 2)
deb-recurse-limit 2,
(Install the actual packages)
deb "qtbase5-dev libgl1-mesa-dev" "./usr/" "./vendor/usr/",
```

All commands must end with a comma at the end.
Comments can be inserted with round parenthesis.

Installing the project is as simple as copying the executable `git-vendor` to the `/bin` or `/usr/bin` or `/usr/local/bin` directory. After installation you can simply cd into the current project dir with a vendor.txt file and run `git-vendor` to pull dependency files.

Building the project should be fairly simple, see the very end for required dependencies.

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
sudo apt install build-essential libbz2-dev liblzma-dev libz-dev libzstd-dev libssl-dev git
```

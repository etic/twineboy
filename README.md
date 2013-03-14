TwineBoy
========

TwineBoy is a tool for converting simple [Twine](http://www.gimcrackd.com/etc/src/) source files into Game Boy ROM images.  TwineBoy is currently in alpha.

Features
--------

Currently, TwineBoy only has support for passages and links.  Code and HTML chunks will be displayed as the raw text.

Installation
------------

Requires python.

``` bash
# download rgbds source code
git clone git://github.com/bentley/rgbds.git

# compile rgbds
cd rgbds
make
sudo make install

# check if rgbasm is installed now
which rgbasm

# download twineboy
git clone https://github.com/etic/twineboy.git
cd twineboy
```

Usage
-----

### Converting a twine story

``` bash
twineboy path/to/twineStory.txt # outputs path/to/twineStory.gb
```

### Controls

- **A:** go to link
- **Up:** scroll passage up
- **Down:** scroll passage down
- **Left:** select previous link
- **Right:** select next link

Changelog
---------

2013 03 14

- First alpha release
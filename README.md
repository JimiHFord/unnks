<div align="center">

# 🎸 unnks 🥁

🎺 NKS and NKX archive unpacker 🎹

📁 ➡️ 🎶 🪕 🎻 🪘 🪗 🎷 🎛️

</div>

## Getting Started

    brew install unnks

:warning: **^ This will not work until this repo gets enough notability to be included into Homebrew ^** :warning:

If you would like to be able to install this via `brew install unnks`, you can help by completing the following:

1. Leave this repo a :star:
   * Homebrew requires GitHub projects to have 75+ :star:s before a formula may be added
2. Fork :fork_and_knife: this repo
   * Homebrew requires GitHub projects to have 30+ :fork_and_knife:s before a formula may be added
3. Watch :eyes: this repo
   * Homebrew requires GitHub projects to have 30+ :eyes:s before a formula may be added


Follow [this PR](https://github.com/Homebrew/homebrew-core/pull/125071) to stay up to date on this project's Homebrew status.

[![Star History Chart](https://api.star-history.com/svg?repos=jimihford/unnks&type=Date)](https://star-history.com/#jimihford/unnks&Date)

After you've picked up your :fork_and_knife:, begun watching :eyes:, and left a :star:, use this temporary solution to get `unnks` installed:

    brew install jimihford/hendrix/unnks

## Introduction

unnks extracts data from nks and nkx archives, which are commonly used by
several music synthesis programs.  It has a similar interface to GNU tar.
Instead of doing

    tar -xvf archive.tar

just do

    unnks -xvf archive.nks
  
or optionally

    unnks -C output_dir -xvf archive.nks

In addition to the unnks program, this package contains the utilities nks-scan,
nks-ls-libs, and the libnks library.

nks-scan can be used to display the structure of a nks/nkx archive.  It was used
during development to discover the meaning of unknown bytes in archives.

nks-ls-libs displays all the sample packages you have installed on your computer
that it can find, and their nkx archive encryption keys.  It is only present on
Windows and Mac OS X.  See the "nkx support" section for how to use this tool to
add support for more nkx archives to unnks.

libnks is a library useful to programmers, which provides an interface to the
code that unnks uses internally to access archives.  If you want to add nks/nkx
support to your GPLv3+ program, then you may use libnks to accomplish it.  Be
warned however, that the API/ABI may change in future releases without warning.

## Building & Installation

See [INSTALL.md](INSTALL.md) for instructions

## nkx support

nkx files are different from nks files in that each archive may have its own
encryption key needed to extract the contents.  These keys are usually supplied
as part of the package containing the archives, but the exact location may vary
and is dependant on the host operating system: on Windows, the keys may be kept
in the registry, on Mac OS X in application property files.  There is no
universal and platform-independent way of accessing these.  This is why unnks
contains a database of known packages with nkx archives and how to obtain their
keys.  Currently, archives from the following packages are supported:

  - Acoustic Legends HD
  - Ambience Impacts Rhythms
  - BASiS
  - Chris Hein Bass
  - Chris Hein - Guitars
  - Chris Hein Horns Vol 2
  - Drums Overkill
  - Ethno World 4
  - Evolve
  - Evolve Mutations
  - Galaxy II
  - Garritan Instruments for Finale
  - Gofriller Cello
  - Keyboard Collection
  - Kreate
  - Mr. Sax T
  - Ocean Way Drums Expandable
  - Ocean Way Drums Gold
  - OTTO
  - Phaedra
  - Prominy SC Electric Guitar
  - Solo Strings Advanced
  - Steven Slate Drums Platinum
  - Stradivari Solo Violin
  - String Essentials
  - Symphobia
  - syntAX
  - The Elements
  - The Trumpet
  - VI.ONE
  - Vir2 Elite Orchestral Percussion

If you want to unpack an unsupported nkx archive, you may try to use the
nks-ls-libs tool supplied with unnks to list the packages installed on your
computer, along with their nkx archive encryption keys.  If you then email this
information to unavowed at vexillium org, it will be included in the next
version of unnks.


## Limitations

1. Only extracting or listing archives is supported; unnks does not create
   new archives.

2. The archives to be extracted must be seekable files.  Extracting from stdin
   or other non-seekable streams is not supported.

## Thanks

Thanks to Gynvael Coldwind for pointing out bugs in the code, and to Steffan
Andrews for all the help with discovering how to extract nkx files and then
testing whether it actually worked.

Also thanks to all those who reported bugs, sent encryption key information and
in general sent emails about this project.

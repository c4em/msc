# msc - Minimal Site Compiler
---

Minimal Site Compiler is a fast and minimalistic static site generator. 

## Installation
### Manual
#### Dependencies
- `md4c`
#### Building
```bash
# Please clone from GitHub to keep the strain on dirae.org low
git clone https://github.com/caemdev/msc
cd msc/
make
```
### Gentoo
EBUILD coming soon

## Usage
### Basic
 - There is a source directory where you store the project files, if no directory
is specified as a argument the current working directory will be set as the
source directory.
 - Compiled files will be copied over to the distribution folder, if no distribution
folder is specified as a argument the a subdirectory called `dist` will be
created in the source directory and set as the distribution directory.
 - Folders starting with `'_'` will be skipped.
 - The source directory structure will be maintained in the distribution folder
 - Markdown files will be converted to html files and be renamed from `.md` to `.html`
 in the distribution directory.
### File embeddings
 - Files can be embedded by specifying `[[[ file.path ]]]` in a html or markdown file.
 - The contents of file `file.path` will be substitued for `[[[ file.path ]]]`.
 - Markdown files will be converted to html
 - This works recursively
### Directory embedding
 - An entire directory can be embedded with `[[[ path/to/directory/* ]]]`
 - Behaviour from section [File embeddings](#file-embeddings) still applies

---
Project hosted on: [https://dirae.org](https://dirae.org)  
Contact: [caem@dirae.org](mailto:caem@dirae.org)  
License: [GPLv3-or-later](https://www.gnu.org/licenses/gpl-3.0.en.html)  


![GPLv3 graphic](https://www.gnu.org/graphics/gplv3-127x51.png)

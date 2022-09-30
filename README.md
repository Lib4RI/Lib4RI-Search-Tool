# Lib4RI Web Search


## Introduction

Goal of this project is to provide a new [web search for Lib4RI](https://search.lib4ri.ch), presenting and partially also combining data fron different online resources.<br>
Finally this feature should be integrated into Lib4RI's redesigned library web site.


## How looks, how it works
In this repository inside the 'info' directory you may find two PowerPoint presentions how this search feature is (currently) intended to look and work.


## Requirements
* Concerning future updates it is recommended not to install [CiteProc PHP](https://github.com/seboettg/citeproc-php) inside the './web' directory directly, but to create a symbolic link called 'citeproc' inside './web' onto the real Citeproc directory.
* Access to APIs (Scopus, WoS, ...)


## Disclaimer
State of the code in this repository is not final, use it at your onw risk.<br>
There is also a strong optimization for the needs of Lib4RI, and confguration options may not be as extensive as possibly desired.<br>
For suggestions, complaints or any feedback else feel free to send an [e-mail](mailto:it.0neDotTooMuch.services@lib4ri.ch).


## License
* [GNU General Public License v3](https://www.gnu.org/licenses/gpl-3.0.en.html)


## To Do
* Content of some tabs must be revised, e.g. links replaced with a result boxes.
* For more precise results some search querries need to be refined.
* Presentation of results (incl. citation style) requires further tuning.
* Quota management for given APIs in not implemented yet.
* Configurability is underdeveloped.
* Search speed and timeout handling should be improved.
* Design will adopt the look of Lib4RI's future library web site.


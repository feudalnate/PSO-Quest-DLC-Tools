# Phantasy Star Online - Quest Installer

Automatically installs PSO quest files for **Xbox** based on external configuration

![Screenshot](https://i.imgur.com/Bb5VTWG.png "")

---

### How to use:

- Place quest files into the root directory of the tool or another directory that's contained within the root directory of the tool
- Configure the `config.ini` for content installation
- Copy the tool and all necessary files and folders to your Xbox
- Run the tool and install the content

---

### Configuring content installation:

> When the Quest Installer is ran, it will expect a `config.ini` file to be present in the root of its directory


Below is an example of a typical `config.ini` file layout

```ini
[pqi.signing]
titleid=4D53004A ; TitleID (8 hexadecimal characters) (for signing ContentMeta.xbx)
titlesignaturekey=E1F49B7D45AFEAA21D5718709828A8B8 ; Title Signature Key (32 hexadecimal characters) (for signing quest files)

;
; Example of quest entries to install (NOTE: Maxiumum of 100 quest entries supported)
;
[pqi.quest.0]              ; Quest entry index (min. index = 0, max. index = 99, inclusive)
file=quests\quest72_e.dat  ; Relative path to quest file (must not start with a backslash \ )
offerid=4D53004A20000048   ; OfferID of content (16 hexadecimal characters)

[pqi.quest.1]
file=quests\quest284_e.dat
offerid=4D53004A2000011C

; Leave a blank line at the end of the .ini file

```
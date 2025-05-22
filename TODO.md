# TODO

## 0.4
- Add batch files to patch examples
- Fix `make clean` not deleting `sm64.map`
- Fill in `sm64.j` asm and RAM/ROM mappings
- Add option to specify output dir
- Name `gen` dir based on config name
- Add `TYPE_M64` to support M64 music data and find references
- Add GNU patch for Windows
- Update libcapstone
- Add some mk64 asm

## 0.5
- Rip models and collision?
- Improve sw `%lo()` detection
- Scan procedure at a time to find instruction pairs to collapse
  - `lui/addiu`, `lui/ori`, `lui/sw` (possibly `.set at` or at least `%hi/%lo`), `slt/bne`
- Add `TYPE_TEXTURE` and support arbitrary types within MIO0 (and other compressed?) blocks
- Add standalone `sm64level`, `sm64behavior` tools?
- Add 0x39 level references to table at EC7E0
- Add way to include jump tables or define data types of tables
  - Autodetect through JR?
- Add RAM labels for tables
- Add VS compiler flags to reduce build size
- Add document or script release
  - Bump version in changed tools
  - Update `release/n64split.README`
  - Update `README` with any changes to usage
  - Create new `build-vM.m.b`
  - `cd build-vM.m.b`
  - `cmake -DCMAKE_BUILD_TYPE=Release ..`
  - Call command line tool to build all projects?
  - Create new `n64split-vM.m.b-win32`
    - Copy `licenses/`
    - Copy `configs/`
    - Copy `release/split.bat`
    - Copy `release/n64split.README.txt` `README.txt`
    - Copy `build-vM.m.b/Release/` tools/
    - Delete `libsm64.lib`
- Add version, help, flags to `n64graphics`
- Add `./test/`
- Remove hacks for overlapping asm procedures
- Create directories for `mipsdisasm`, `libmio0`
- Merge `rawmips` into `mipsdisasm`
- Rename `mipsdisasm` to `mipsrdisasm` - MIPS recursive disasm
- Add parens and arguments in function name
- Use segmented addresses for more level script callouts
  - Move data to segmented address (possibly overlay)
  - Add symbols for LOAD addresses
- Add asm macros for level scripts
- Add asm macros for geo layout
  - Add labels for asm and other references
- Add asm macros for behavior scripts
- Create common label database (config labels, ROM labels, behavior labels, HW register labels, jump tables)
- Update linker script for RAM/ROM AT addressing
- Find way to have data labels
  - In config file
  - Linker script
- Figure out remaining cut scenes

### Maybe
- Use shygoo's 0x1A level command with uncompressed data
- Convert texture encoding to:
  - split → `textures/A.0x0.png`, `A.s`
  - `textures/A.0x0.png --n64graphics--> bin/textures/A.0x0.bin`
  - `.include "A.s" .incbin "bin/textures/A.0x0.bin"`
  - `A.bin -> A.mio0`: remove this step

## Examples
- Hello world example
  - Hook into mario function instead of butterfly behavior
- Hello world 2 example  
  [http://www.hastebin.com/unuqiwetov](http://www.hastebin.com/unuqiwetov) ← use this one  
  [http://www.hastebin.com/ayoqopoyeq](http://www.hastebin.com/ayoqopoyeq)
- Switch to Green Stars method of HUD toggle
  - Still need show star status
- Look into what importer does for bounds extending
- Import custom levels
- Look into m64 music importing
- Add new `.text` section at `0x80400000` and setup DMAs for it

## Other
- Add f3d parsing?
- See what else the level importer changed
- Figure out where music and samples are located and split out
- Switch emulator to cen64
- See what data is after geo layout after main_level_scripts
- Improve mio0 compressor
- Check capstone disassembly of COP instructions
  - Appears to do some COP2, need to validate assembly again
  - Does not do RSP, need to implement in custom callback for data
    - Probably also requires switching to callback for each instruction  
      [http://sprunge.us/VYYI?gas](http://sprunge.us/VYYI?gas)

## Figure out what these functions are for
### JALs
- Recursive, only called by self `802C9AD8`  
  Looking for `802C9AD8 0C0B26B6`  
  `084AFC: 0C0B26B6`  
  `084B28: 0C0B26B6`

- Recursive, only called by self `8017E430`  
  Looking for `8017E430 0C05F90C`  
  `22E96C: 0C05F90C`

- Recursive, only called by self `8017F350`  
  Looking for `8017F350 0C05FCD4`  
  `22F96C: 0C05FCD4`

- Recursive, only called by self `8018837C`  
  Looking for `8018837C 0C0620DF`  
  `238968: 0C0620DF`

- Looking for `8017C810 0C05F204`  
  `257A28: 0C05F204`

- Looking for `8017EF9C 0C05FBE7`  
  `257B88: 0C05FBE7`

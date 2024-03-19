# luamemzip
Lua library to create and manipulate zip files in memory

Tries to do replicate the API from this project as closely as possible: <https://github.com/kuba--/zip>

## Usage
```lua
-- Loads the luamemzip.dll
zip = require("luamemzip")

content = "Test content"

z = zip.zip_stream_open(nil, 6, 'w')

zip.zip_entry_open(z, "test.content")
zip.zip_entry_write(z, content)
zip.zip_entry_close(z)

data = zip.zip_stream_copy(z)

zip.zip_close(z)
```

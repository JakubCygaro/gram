1. Better visuals 
    - min max values
    - grid marks
    - maybe value inspection per datapoint (on mouse hover)
2. Screenshot generation
3. Build in csv parsing and loading for lua (create `libgram_csv`)

Something akin to `gram.load_csv(path: string)` that would load a file and return it as a table
```
{
    fname: string -- name of the file,
    "headername" : { table of data },
}
```

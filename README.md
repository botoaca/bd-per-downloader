# Biblioteca Deva Periodice Downloader
this utility was build to help mass-download files from the periodical section of the B.J.O.D.HD digital library.

## Usage
| Command-line argument | Purpose |
|-----------------------|---------|
| --src | The (code)name of the publication you wish to download |
| --start (optional) | The start of the time period you wish to download from (YYYY/MM) |
| --end | The end of the time period you wish to download from (YYYY/MM) |
| --verbose | Prints which files are being downloaded |
| --wait | Lets you wait for *x* seconds to avoid overloading the server |

if you only wish to download one month of issues, you can omit the --start argument altogether and set the --end argument to the date of the one that should be downloaded\
*examples*:
```
./bd-per-downloader --src scanteia --start 1968/05 --end 1968/06 (will download every issue from may to june 1968)
./bd-per-downloader --src scanteiatin --end 1954/01 (will download every issue from january 1954)
./bd-per-downloader --src albina --start 0/0 --end 9999/9999 --verbose > log.txt (will download every single issue and save a log)
./bd-per-downloader --src femeia --start 0/0 --end 9999/9999 --wait 5 (will download every single issue, one every 5 seconds)
```

### Where do I get the publication source source?
you can see all available sources at [THE PERIODICALS AND SERIAL PUBLICATIONS collection of the "Ovid Densusianu" County Library Hunedoara-Deva](http://bibliotecadeva.eu:82/periodice/periodice.html)\
*examples*: `scanteia` (Scînteia), `scanteiatin` (Scînteia Tineretului), `albina` (Albina), ...\

keep in mind that some *codenames* are different from the *name* of the publication. however, you can easily find the codename in the url:\
the url for Agricultura Socialistă is *http://bibliotecadeva.eu:82/periodice/agriculturas.html* - and the *codename* is "agriculturas"
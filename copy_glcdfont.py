# Przed kompilacją podmienia czcionkę 5x7 biblioteki Adafruit GFX na wersję z polskimi znakami z locale/glcdfont.
# utf8Pol.cpp zamienia ą, ę, ł itd. na kody 0x80-0xFF, które ma tylko ta czcionka; zwykła czcionka GFX
# pokazuje w ich miejscu inne symbole. Plik w .pio/libdeps znika przy każdej ponownej instalacji biblioteki,
# dlatego jest kopiowany przy każdej kompilacji.
Import("env")
import os, shutil, filecmp

src = os.path.join(env.subst("$PROJECT_DIR"), "locale", "glcdfont", "PL, SK, DE, RO, S, A", "glcdfont.c")
dst = os.path.join(env.subst("$PROJECT_LIBDEPS_DIR"), env.subst("$PIOENV"), "Adafruit GFX Library", "glcdfont.c")

if not os.path.isfile(dst):
    print("copy_glcdfont: brak biblioteki Adafruit GFX w libdeps, pomijam")
elif not filecmp.cmp(src, dst, shallow=False):
    shutil.copyfile(src, dst)
    print("copy_glcdfont: czcionka z polskimi znakami skopiowana do Adafruit GFX")

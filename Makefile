PRJNAME := winter_shooter
OBJS := data.rel actor.rel shot.rel shots.rel map.rel score.rel winter_shooter.rel

all: $(PRJNAME).sms

data.c: data/* data/sprites_tiles.psgcompr data/tileset_tiles.psgcompr \
		data/path1.path data/path2.path data/path3.path data/path4.path data/level1.bin \
		data/player_shot.psg data/enemy_death.psg data/get_powerup.psg
	folder2c data data
	
data/sprites_tiles.psgcompr: data/img/sprites.png
	BMP2Tile.exe data/img/sprites.png -noremovedupes -8x16 -palsms -fullpalette -savetiles data/sprites_tiles.psgcompr -savepalette data/sprites_palette.bin
	
data/tileset_tiles.psgcompr: data/img/tileset.png
	BMP2Tile.exe data/img/tileset.png -noremovedupes -8x16 -palsms -fullpalette -savetiles data/tileset_tiles.psgcompr -savepalette data/tileset_palette.bin

data/%.path: data/path/%.spline.json
	node tool/convert_splines.js $< $@

data/%.bin: data/map/%.tmx
	node tool/convert_map.js $< $@

%.vgm: %.wav
	psgtalk -r 512 -u 1 -m vgm $<

%.rel : %.c
	sdcc -c -mz80 --peep-file lib/peep-rules.txt $<

$(PRJNAME).sms: $(OBJS)
	sdcc -o $(PRJNAME).ihx -mz80 --no-std-crt0 --data-loc 0xC000 lib/crt0_sms.rel $(OBJS) SMSlib.lib lib/PSGlib.rel
	ihx2sms $(PRJNAME).ihx $(PRJNAME).sms	

clean:
	rm *.sms *.sav *.asm *.sym *.rel *.noi *.map *.lst *.lk *.ihx data.*

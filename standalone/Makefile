# compile the shared object file
so:
	clang -shared ../queue.c ../qsort.c ../tin.c -o tin.so

srtm_12_05:
	wget http://srtm.csi.cgiar.org/SRT-ZIP/SRTM_V41/SRTM_Data_GeoTiff/srtm_12_05.zip
	unzip srtm_12_05.zip -d srtm_12_05

# gdalwarp srtm to desired projection, extract area you want
# TODO: pass latlong extent, translate to web mercator or UTM
# sf.tif: srtm_12_05
#	gdalwarp -nomd -t_srs EPSG:4326 -te -122.53086 37.69523 -122.35233 37.81466 srtm_12_05/srtm_12_05.tif sf.tif

# gdal_translate to ArcGrid
srtm_12_05.grid: srtm_12_05
	gdal_translate srtm_12_05/srtm_12_05.tif -of AAIGrid srtm_12_05.grid

# 5% of max elev error allowed
srtm_12_05.tin: srtm_12_05.grid
	../r.refine srtm_12_05.grid srtm_12_05.tin 5

read:
	# read the TIN
	python tin.py	srtm_12_05.tin

clean:
	rm srtm_12_05.grid*
	rm srtm_12_05.proj
	rm srtm_12_05.tin

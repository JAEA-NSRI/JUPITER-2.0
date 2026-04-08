
.PHONY: help clean pack2txt make-vtk

help:
	@echo "Available targets are:"
	@echo "  * make-vtk"
	@echo "  * pack2txt"

make-vtk: make-vtk/make-vtk
pack2txt: pack2txt/pack2txt

make-vtk/mak-vtk:
	$(MAKE) $(MFLAGS) -C make-vtk make-vtk

pack2txt/pack2txt:
	$(MAKE) $(MFLAGS) -C pack2txt pack2txt

extfile-extractor/jupiter-extfile-extractor:
	$(MAKE) $(MFLAGS) -C extfile-extractor jupiter-extfile-extractor

clean:
	$(MAKE) $(MFLAGS) -C pack2txt clean
	$(MAKE) $(MFLAGS) -C make-vtk clean
	$(MAKE) $(MFLAGS) -C extfile-extractor clean

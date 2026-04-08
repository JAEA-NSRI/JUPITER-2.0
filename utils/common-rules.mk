
$(JUPITER_LIB): $(JUPITER_LIB_FULL)
	cp $(JUPITER_LIB_FULL) $(JUPITER_LIB)

$(JUPITER_LIB_FULL):
	$(MAKE) $(MFLAGS) -C $(JUPITER) $(JUPITER_LIB)

$(GEOMETRY_LIB): $(GEOMETRY_LIB_FULL)
	cp $(GEOMETRY_LIB_FULL) $(GEOMETRY_LIB)

$(GEOMETRY_LIB_FULL):
	$(MAKE) $(MFLAGS) -C $(GEOMETRY) $(GEOMETRY_LIB)

$(SERIALIZER_LIB): $(SERIALIZER_LIB_FULL)
	cp $(SERIALIZER_LIB_FULL) $(SERIALIZER_LIB)

$(SERIALIZER_LIB_FULL):
	$(MAKE) $(MFLAGS) -C $(SERIALIZER) $(SERIALIZER_LIB)

$(TABLE_LIB): $(TABLE_LIB_FULL)
	cp $(TABLE_LIB_FULL) $(TABLE_LIB)

$(TABLE_LIB_FULL):
	$(MAKE) $(MFLAGS) -C $(TABLE) $(TABLE_LIB)

%.o: %.c
	$(CC) $(CFLAGS)	$(INCLUDES) $(DEFS) -c -o $@ $<

jupiter-all-clean:
	$(MAKE) -C $(JUPITER) clean

geometry-lib-clean:
	$(MAKE) -C $(GEOMETRY) clean

serializer-lib-clean:
	$(MAKE) -C $(SERIALIZER) clean

table-lib-clean:
	$(MAKE) -C $(TABLE) clean

.PHONY: jupiter-all-clean geometry-lib-clean serializer-lib-clean table-lib-clean

GLSLC=glslc


SOURCES_GLSL=$(wildcard *.glsl)
SOURCES_VERT=$(wildcard *.vert)
SOURCES_FRAG=$(wildcard *.frag)
OBJECTS=$(SOURCES_VERT:%=%.spv) $(SOURCES_VERT:%=%.depth.spv) $(SOURCES_FRAG:%=%.spv)


all: $(OBJECTS)


%.frag.spv: %.frag $(SOURCES_GLSL)
	$(GLSLC) -O $< -o $@

%.vert.spv: %.vert $(SOURCES_GLSL)
	$(GLSLC) -O $< -o $@

%.vert.depth.spv: %.vert $(SOURCES_GLSL)
	$(GLSLC) -DDEPTH_ONLY -O $< -o $@


clean:
	-rm *.spv


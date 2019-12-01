SHELL=/bin/bash -o pipefail

DOCKER ?= docker

.DEFAULT_GOAL := all

PANDOC_BUILDER_IMAGE ?= "quay.io/dalehamel/pandoc-report-builder"
PWD ?= `pwd`

clean:
	rm -rf output
	rm -f index.html

.PHONY: doc/build
doc/build:
	${DOCKER} run --user `id -u`:`id -g` -v ${PWD}:/app ${PANDOC_BUILDER_IMAGE} /app/scripts/pandoc-build

output/doc.md:
	${DOCKER} run --user `id -u`:`id -g` -v ${PWD}:/app ${PANDOC_BUILDER_IMAGE} pandoc doc.docx -o output/doc.md

index.html:
	ln -sf output/doc.html index.html

.PHONY: quirks
quirks:
	scripts/tidy

.PHONY: audio
audio:
	scripts/make-audio

all: doc/build index.html

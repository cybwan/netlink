#!make

.PHONY: format-c
format-c:
	find . -regex '.*\.\(c\|h\)' -exec clang-format -style=file -i {} \;

.PHONY: gonl-build
gonl-build:
	@CGO_ENABLED=1 go build -v -o ./bin/gonl ./cmd/gonl/*

.PHONY: gonl-run
gonl-run:
	@./bin/gonl

.PHONY: gonl
gonl: gonl-build gonl-run

.PHONY: nlp
nlp:
	cd nlp && make

.PHONY: nlp-run
nlp-run: nlp
	cd nlp && make run

.PHONY: nlp-clean
nlp-clean:
	cd nlp && make distclean
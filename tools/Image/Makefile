build: 
	$(CARGO) $(CARGOFLAGS) --manifest-path Cargo.toml >/dev/null 2>&1
	cp target/release/image $(OUTPUT)/image
	printf "\033[1m\033[33m[IMAGE]      \033[92mCreated -> image\033[0m\n"
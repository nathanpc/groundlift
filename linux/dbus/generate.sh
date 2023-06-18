#!/bin/sh

gdbus-codegen \
	--interface-prefix com.innoveworkshop \
	--generate-c-code gdbus \
	./com.innoveworkshop.Groundlift.xml

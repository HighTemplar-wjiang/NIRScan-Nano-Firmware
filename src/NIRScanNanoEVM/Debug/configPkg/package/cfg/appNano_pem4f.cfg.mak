# invoke SourceDir generated makefile for appNano.pem4f
appNano.pem4f: .libraries,appNano.pem4f
.libraries,appNano.pem4f: package/cfg/appNano_pem4f.xdl
	$(MAKE) -f C:\Users\weiwei\workspace_v6_2\MobileSpectroscopyTIVAEVM/src/makefile.libs

clean::
	$(MAKE) -f C:\Users\weiwei\workspace_v6_2\MobileSpectroscopyTIVAEVM/src/makefile.libs clean


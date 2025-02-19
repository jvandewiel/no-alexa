# issues with userdata

- some index branches fail -> reading error from flash? issues at source of index node?
    - offset 56?

    16 + 4080 = 4096 _ assume we need to remove the 0xFF and only keep the real data

    532480 - 141557759 -> hsqs
    hsqs is also in UBI blocks, check if every time 8192 bytes offset and then data -> concat, lzo/decomp?
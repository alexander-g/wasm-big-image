import { asserts } from "./dep.ts"

import { BigImage, ImageSize, Image, initialize } from "../src/wasm-big-image.ts"



const TIFF_FILE = "tests/assets/sheep.tiff";



Deno.test("get_tiff_size", async () => {
    const data_u8:Uint8Array = Deno.readFileSync(TIFF_FILE)
    const tiffile:File = new File([data_u8.buffer as ArrayBuffer], "sheep.tiff")

    const module:BigImage|Error = await initialize()
    const size: Error|ImageSize = await module.get_tiff_size(tiffile);
    asserts.assertEquals(size, {width:242, height:168});

})



Deno.test("tiff_read", async () => {
    const data_u8:Uint8Array = Deno.readFileSync(TIFF_FILE)
    const tiffile:File = new File([data_u8.buffer as ArrayBuffer], "sheep.tiff")

    const module:BigImage|Error = await initialize()
    const image: Error|Image = await module.tiff_read(tiffile);
    asserts.assertNotInstanceOf(image, Error)

    asserts.assertEquals(image.data.length, 242*168*4);
})



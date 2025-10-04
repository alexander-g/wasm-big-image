import { asserts } from "./dep.ts"

import { BigImage, ImageSize, initialize } from "../src/wasm-big-image.ts"



const TIFF_FILE = "tests/assets/sheep.tiff";



Deno.test("read_tiff", async () => {
    const data_u8:Uint8Array = Deno.readFileSync(TIFF_FILE)

    const module:BigImage|Error = await initialize()
    const size: Error|ImageSize = await module.get_tiff_size(data_u8);
    asserts.assertEquals(size, {width:242, height:168});

})





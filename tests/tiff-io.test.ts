import { asserts } from "./dep.ts"

// @deno-types="../src/wasm-big-image.d.ts"
import { BigImage, ImageSize, Image, initialize } from "../src/wasm-big-image.ts"



const TIFF_FILE = "tests/assets/sheep.tiff";



Deno.test("image_get_size", async () => {
    const data_u8:Uint8Array = Deno.readFileSync(TIFF_FILE)
    const tiffile:File = new File([data_u8.buffer as ArrayBuffer], "sheep.tiff")

    const module:BigImage|Error = await initialize()
    const size: Error|ImageSize = await module.image_get_size(tiffile);
    asserts.assertEquals(size, {width:242, height:168});

    // actual bug: calling twice in a row gives invalid values
    const size2: Error|ImageSize = await module.image_get_size(tiffile);
    asserts.assertEquals(size2, {width:242, height:168});
})


Deno.test("image_read_patch", async () => {
    const data_u8:Uint8Array = Deno.readFileSync(TIFF_FILE)
    const tiffile:File = new File([data_u8.buffer as ArrayBuffer], "sheep.tiff")

    const module:BigImage|Error = await initialize()
    const image: Error|Image = 
        await module.image_read_patch(tiffile, 50,140,25,25, 25, 25);
    asserts.assertNotInstanceOf(image, Error)
    asserts.assertEquals(image.data.length, 25*25*4);

    // actual bug: memory issues
    const image2: Error|Image = 
        await module.image_read_patch(tiffile, 50,50,50,50, 50,50);
    asserts.assertNotInstanceOf(image2, Error)


    const image3: Error|Image = 
        await module.image_read_patch(tiffile, 50,140,25,25, 5, 5);
    asserts.assertNotInstanceOf(image3, Error)
    asserts.assertEquals(image3.data.length, 5*5*4);

    const image4: Error|Image = 
        await module.image_read_patch(tiffile, 50,140,25,25, 98, 99);
    asserts.assertNotInstanceOf(image4, Error)
    asserts.assertEquals(image4.data.length, 98*99*4);
    asserts.assertEquals(image4.width, 98)
    asserts.assertEquals(image4.height, 99)
})


Deno.test("image_read_patch_and_encode", async() => {
    const data_u8:Uint8Array = Deno.readFileSync(TIFF_FILE)
    const tiffile:File = new File([data_u8.buffer as ArrayBuffer], "sheep.tiff")

    const module:BigImage|Error = await initialize()
    const image: Error|File = 
        await module.image_read_patch_and_encode(tiffile, 50,140,25,25, 25,25, false);
    
    asserts.assertNotInstanceOf(image, Error)
})




const IMAGEPATH_JPEG = "tests/assets/jpeg0.jpg";

Deno.test('image_read_patch: jpeg', async () => {
    const imagefile = new File([Deno.readFileSync(IMAGEPATH_JPEG)], 'file.jpg')
    const module:BigImage|Error = await initialize()
    const image: Error|Image = 
        await module.image_read_patch(imagefile, 0,0,400,400, 250, 250);
    asserts.assertNotInstanceOf(image, Error)
})


const IMAGEPATH_PNG = "tests/assets/png0.png";

Deno.test('image_read_patch: png', async () => {
    const imagefile = new File([Deno.readFileSync(IMAGEPATH_PNG)], 'file.png')
    const module:BigImage|Error = await initialize()
    const image: Error|Image = 
        await module.image_read_patch(imagefile, 0,0,400,400, 250, 250);
    asserts.assertNotInstanceOf(image, Error)
})



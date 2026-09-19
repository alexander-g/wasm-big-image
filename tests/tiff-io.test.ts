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


Deno.test("image_read_patch: tiff", async (t:Deno.TestContext) => {
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



    await t.step('patch-larger-than-image', async () => {
        const image: Error|Image = 
            await module.image_read_patch(tiffile, 0,0,300,300, 300, 300);
        asserts.assertNotInstanceOf(image, Error)
        asserts.assertEquals(image.width, 300)
        asserts.assertEquals(image.height, 300)

        // og image size: 242x168
        // should be padded with opaque black (alpha channel doesnt matter)
        for(let i = 0; i < image.height; i++)
            for(let j = (i >= 168)? 0 : 242 ; j < image.width; j++)
                // rgb channels
                for(let c = 0; c < 3; c++)
                    asserts.assertEquals(image.data[ i*image.width*4 +  j*4 + c ], 0, `${i}-${j}-${c}`)
    })
})


Deno.test("image_read_patch_and_encode", async() => {
    const data_u8:Uint8Array = Deno.readFileSync(TIFF_FILE)
    const tiffile:File = new File([data_u8.buffer as ArrayBuffer], "sheep.tiff")

    const module:BigImage|Error = await initialize()
    const image: Error|File = 
        await module.image_read_patch_and_encode(tiffile, 50,140,25,25, 25,25, false);
    
    asserts.assertNotInstanceOf(image, Error)
})



// image size: 444x777
const IMAGEPATH_JPEG = "tests/assets/jpeg1.jpg";

Deno.test('image_read_patch: jpeg', async (t:Deno.TestContext) => {
    const imagefile = new File([Deno.readFileSync(IMAGEPATH_JPEG)], 'file.jpg')
    const module:BigImage|Error = await initialize()

    await t.step('basic',  async () => {
        const image: Error|Image = 
            await module.image_read_patch(imagefile, 0,0,400,400, 250, 250);
        asserts.assertNotInstanceOf(image, Error)
        asserts.assertEquals(image.width, 250)
        asserts.assertEquals(image.height, 250)
    } )

    await t.step('patch-larger-than-image', async () => {
        const image: Error|Image = 
            await module.image_read_patch(imagefile, 0,0,800,800, 800, 800);
        asserts.assertNotInstanceOf(image, Error)
        asserts.assertEquals(image.width, 800)
        asserts.assertEquals(image.height, 800)


        // should be padded with opaque black (alpha channel doesnt matter)
        for(let i = 0; i < image.height; i++)
            for(let j = (i >= 777)? 0 : 444 ; j < image.width; j++)
                // rgb channels
                for(let c = 0; c < 3; c++)
                    asserts.assertEquals(image.data[ i*image.width*4 +  j*4 + c ], 0, `${i}-${j}-${c}`)
    })
})


const IMAGEPATH_PNG = "tests/assets/png0.png";

Deno.test('image_read_patch: png', async () => {
    const imagefile = new File([Deno.readFileSync(IMAGEPATH_PNG)], 'file.png')
    const module:BigImage|Error = await initialize()
    const image: Error|Image = 
        await module.image_read_patch(imagefile, 0,0,400,400, 250, 250);
    asserts.assertNotInstanceOf(image, Error)
})



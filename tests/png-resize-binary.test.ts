import { asserts } from './dep.ts'

// @deno-types="../src/wasm-big-image.d.ts"
import { BigImage, initialize } from '../src/wasm-big-image.ts'


Deno.test('resize_image_and_encode_as_png_binary_expected_usage', async () => {
    const module: BigImage | Error = await initialize()
    asserts.assertNotInstanceOf(module, Error)

    const mask_data: Uint8Array = new Uint8Array([
        1, 0,
        0, 1,
    ])

    const encoded: Uint8Array | Error =
        await module.resize_image_and_encode_as_png_binary(
            mask_data,
            2,
            2,
            4,
            4,
        )

    asserts.assertNotInstanceOf(encoded, Error)
    asserts.assertEquals(encoded[0], 0x89)
    asserts.assertEquals(encoded[1], 0x50)
    asserts.assertEquals(encoded[2], 0x4e)
    asserts.assertEquals(encoded[3], 0x47)
})


Deno.test('resize_image_and_encode_as_png_binary_edge_empty_mask', async () => {
    const module: BigImage | Error = await initialize()
    asserts.assertNotInstanceOf(module, Error)

    const encoded: Uint8Array | Error =
        await module.resize_image_and_encode_as_png_binary(
            new Uint8Array(),
            0,
            0,
            1,
            1,
        )

    asserts.assertInstanceOf(encoded, Error)
})


Deno.test('resize_image_and_encode_as_png_binary_failure_size_mismatch', async () => {
    const module: BigImage | Error = await initialize()
    asserts.assertNotInstanceOf(module, Error)

    const encoded: Uint8Array | Error =
        await module.resize_image_and_encode_as_png_binary(
            new Uint8Array([1, 0, 1]),
            2,
            2,
            2,
            2,
        )

    asserts.assertInstanceOf(encoded, Error)
})

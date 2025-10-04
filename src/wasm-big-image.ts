


export type ImageSize = {width:number, height:number}



// for better readability
type fn_pointer = number;
type pointer = number;

type BigImageWASM = {
    _tiff_get_size: (
        filesize:             number,
        read_file_callback_p: fn_pointer,
        read_file_handle:     number,
        width_p:   pointer,
        height_p:  pointer,
        tif_p:   0,
    ) => number,
    _malloc: (nbytes:number) => pointer,
    _free:   (ptr:pointer) => void,

    // deno-lint-ignore no-explicit-any
    addFunction: ( fn: (...args: any[]) => unknown, argtypes:string ) => number,
    
    HEAPU8: {
        set: (src:Uint8Array, dst:pointer) => void,
        [i:number]: number,
    }
    HEAP32: {
        [i:number]: number,
    },
    HEAP64: {
        [i:number]: bigint,
    }
}


export class BigImage {
    constructor(private wasm:BigImageWASM) {
        this.#read_file_callback_ptr = 
            wasm.addFunction(this.#read_file_callback, 'iiiii');
    }

    get_tiff_size(file:Uint8Array): ImageSize|Error {
        const handle:number = this.#handle_counter++;
        this.#read_file_callback_table[handle] = file;

        const ptr:number = this.wasm._malloc(16);
        const w_ptr:number = ptr + 0;
        const h_ptr:number = ptr + 8;
        const rc:number = this.wasm._tiff_get_size(
            file.length, 
            this.#read_file_callback_ptr, 
            handle, 
            w_ptr, 
            h_ptr, 
            0
        )
        
        const w:number = Number(this.wasm.HEAP64[w_ptr >> 3])
        const h:number = Number(this.wasm.HEAP64[h_ptr >> 3])

        this.wasm._free(ptr);
        delete this.#read_file_callback_table[handle];
        
        if(rc != 0)
            return new Error('Reading image size failed')
        return {width:w, height:h}
    }


    #handle_counter = 0;

    #read_file_callback_ptr:pointer;
    #read_file_callback_table: Record<number, Uint8Array> = {};
    #read_file_callback = (
        handle: number,
        dstbuf: pointer,
        start:  number,
        size:   number,
    ): number  => {
        const filebytes:Uint8Array|undefined = this.#read_file_callback_table[handle];
        if(!filebytes)
            return -1;
        
        const slice_u8:Uint8Array = filebytes.slice(start, start+size);
        this.wasm.HEAPU8.set(slice_u8, dstbuf);
        return 0;
    }
}



export async function initialize(): Promise<BigImage> {
    const wasm:BigImageWASM = 
        await (await import('../build-wasm/big-image.js')).default();
    
    return new BigImage(wasm);
}







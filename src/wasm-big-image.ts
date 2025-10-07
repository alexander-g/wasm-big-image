


export type ImageSize = {width:number, height:number}
export type Image = {data:Uint8Array} & ImageSize;



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

    _tiff_read: (
        filesize:             number,
        read_file_callback_p: fn_pointer,
        read_file_handle:     number,
        buffer:               pointer,
        buffersize:           number,
    ) => number,

    _malloc: (nbytes:number) => pointer,
    _free:   (ptr:pointer) => void,

    // deno-lint-ignore no-explicit-any
    addFunction: ( fn: (...args: any[]) => unknown, argtypes:string ) => number,
    
    HEAPU8: {
        set: (src:Uint8Array, dst:pointer) => void,
        slice: (start:number, end:number) => Uint8Array,
        [i:number]: number,
    }
    HEAP32: {
        [i:number]: number,
    },
    HEAP64: {
        [i:number]: bigint,
    },

    Asyncify: {
        // deno-lint-ignore no-explicit-any
        handleAsync: (fn:(...args:any[]) => Promise<unknown>) => unknown,
        currData: number|null,
    },

}

export function wait(ms: number): Promise<unknown> {
    return new Promise((resolve) => {
        setTimeout(() => resolve(0), ms)
    })
}




export class BigImage {
    constructor(private wasm:BigImageWASM) {
        this.#read_file_callback_ptr = 
            wasm.addFunction(this.#read_file_callback, 'iiiii');
    }

    async get_tiff_size(file:File): Promise<ImageSize|Error> {
        const handle:number = this.#handle_counter++;
        this.#read_file_callback_table[handle] = file;

        const ptr:number = this.wasm._malloc(16);
        const w_ptr:number = ptr + 0;
        const h_ptr:number = ptr + 8;
        const rc:number = await this.wasm._tiff_get_size(
            file.size, 
            this.#read_file_callback_ptr, 
            handle, 
            w_ptr, 
            h_ptr, 
            0
        )
        // NOTE: the wasm function above returns before the execution is 
        // finished because of async issues, so currently polling until done
        while(this.wasm.Asyncify.currData != null)
            await wait(1);
        
        
        const w:number = Number(this.wasm.HEAP64[w_ptr >> 3])
        const h:number = Number(this.wasm.HEAP64[h_ptr >> 3])

        this.wasm._free(ptr);
        delete this.#read_file_callback_table[handle];
        
        // TODO: rc is invalid!
        if(rc != 0)
            return new Error('Reading image size failed')
        return {width:w, height:h}
    }

    async tiff_read(file:File): Promise<Image|Error> {
        // TODO: this reads the image size twice, bc wasm._tiff_read also does
        const imsize:ImageSize|Error = await this.get_tiff_size(file)
        if(imsize instanceof Error)
            return imsize as Error;
        
        const handle:number = this.#handle_counter++;
        this.#read_file_callback_table[handle] = file;

        const nbytes:number = imsize.width * imsize.height * 4;
        const buffer:pointer = this.wasm._malloc(nbytes)
        const rc:number = this.wasm._tiff_read(
            file.size, 
            this.#read_file_callback_ptr, 
            handle, 
            buffer, 
            nbytes,
        )
        // NOTE: the wasm function above returns before the execution is 
        // finished because of async issues, so currently polling until done
        while(this.wasm.Asyncify.currData != null)
            await wait(1);
        
        // copy
        const rgba:Uint8Array = this.wasm.HEAPU8.slice(buffer, buffer+nbytes)
        
        this.wasm._free(buffer);
        delete this.#read_file_callback_table[handle];
        
        // TODO: rc is invalid!
        if(rc != 0)
            return new Error('Reading tiff file failed')
        return {data:rgba, ...imsize};
    }




    #handle_counter = 0;

    #read_file_callback_ptr:pointer;
    #read_file_callback_table: Record<number, File> = {};

    /** Called by WASM to read a required portion of a file */
    #read_file_callback = (
        handle: number,
        dstbuf: pointer,
        start:  number,
        size:   number,
    ): unknown  => {
        return this.wasm.Asyncify.handleAsync( async () => {
            const file:File|undefined = 
                this.#read_file_callback_table[handle];
            if(!file)
                return -1;
            
            const slice_u8:Uint8Array = new Uint8Array(
                await file.slice(start, start+size).arrayBuffer()
            )
            this.wasm.HEAPU8.set(slice_u8, dstbuf);
            return 0;
        })
    }
}



export async function initialize(): Promise<BigImage> {
    const wasm:BigImageWASM = 
        // deno-lint-ignore no-explicit-any
        await (await import('../build-wasm/big-image.js')).default() as any;

    //console.log(Object.keys(wasm.Asyncify))
    
    return new BigImage(wasm);
}



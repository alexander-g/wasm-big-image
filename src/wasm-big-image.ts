import type {
    initialize as Iinitialize,
    BigImage as IBigImage,
    ImageSize,
    Image,
} from "./wasm-big-image.d.ts"





// for better readability
type fn_pointer = number;
type pointer = number;

type BigImageWASM = {
    
    _image_get_size: (
        filesize:             number,
        read_file_callback_p: fn_pointer,
        read_file_handle:     number,
        width_p:   pointer,
        height_p:  pointer,
        returncode: pointer,
    ) => number,

    _image_read_patch: (
        filesize:             number,
        read_file_callback_p: fn_pointer,
        read_file_handle:     number,
        src_x:                number,
        src_y:                number,
        src_width:            number,
        src_height:           number,
        dst_width:            number,
        dst_height:           number,
        dst_buffer:           pointer,
        dst_buffersize:       number,
        returncode:           pointer,
    ) => number,

    _image_read_patch_and_encode: (
        filesize:             number,
        read_file_callback_p: fn_pointer,
        read_file_handle:     number,
        src_x:                number,
        src_y:                number,
        src_width:            number,
        src_height:           number,
        dst_width:            number,
        dst_height:           number,
        lossless:             boolean,
        output_buffer:        pointer,
        output_size:          pointer,
        returncode:           pointer,
    ) => number,

    _free_output_buffer: (buffer_p:pointer) => number,


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




export class BigImage implements IBigImage  {
    constructor(private wasm:BigImageWASM) {
        this.#read_file_callback_ptr = 
            wasm.addFunction(this.#read_file_callback, 'iiijj');
    }

    async image_get_size(file:File): Promise<ImageSize|Error> {
        const handle:number = this.#handle_counter++;
        this.#read_file_callback_table[handle] = file;

        const w_ptr:number = this.wasm._malloc(8);
        const h_ptr:number = this.wasm._malloc(8);
        this.wasm.HEAP64[w_ptr >> 3] = 0n;
        this.wasm.HEAP64[h_ptr >> 3] = 0n;
        const rc_ptr:pointer = this.wasm._malloc(4)
        this.wasm.HEAP32[rc_ptr >> 2] = 777;

        let rc:number = await this.wasm._image_get_size(
            file.size, 
            this.#read_file_callback_ptr, 
            handle, 
            w_ptr, 
            h_ptr, 
            rc_ptr
        )
        // NOTE: the wasm function above returns before the execution is 
        // finished because of async issues, so currently polling until done
        while(this.wasm.Asyncify.currData != null)
            await wait(1);
        
        
        const w:number = Number(this.wasm.HEAP64[w_ptr >> 3])
        const h:number = Number(this.wasm.HEAP64[h_ptr >> 3])
        rc = (rc == 0)? this.wasm.HEAP32[rc_ptr >> 2]! : rc;

        this.wasm._free(w_ptr);
        this.wasm._free(h_ptr);
        this.wasm._free(rc_ptr);
        delete this.#read_file_callback_table[handle];
        
        // TODO: rc is invalid!
        if(rc != 0)
            return new Error('Reading image size failed')
        return {width:w, height:h}
    }

    async image_read_patch(
        file:       File, 
        src_x:      number, 
        src_y:      number,
        src_width:  number,
        src_height: number,
        dst_width:  number,
        dst_height: number,
    ): Promise<Image|Error> {
        const handle:number = this.#handle_counter++;
        this.#read_file_callback_table[handle] = file;

        const nbytes:number = dst_width * dst_height * 4;
        const buffer:pointer = this.wasm._malloc(nbytes)
        const rc_ptr:pointer = this.wasm._malloc(4)
        this.wasm.HEAP32[rc_ptr >> 2] = 777;
        let rc:number = this.wasm._image_read_patch(
            file.size, 
            this.#read_file_callback_ptr, 
            handle, 
            src_x,
            src_y,
            src_width,
            src_height,
            dst_width,
            dst_height,
            buffer, 
            nbytes,
            rc_ptr
        )
        // NOTE: the wasm function above returns before the execution is 
        // finished because of async issues, so currently polling until done
        while(this.wasm.Asyncify.currData != null)
            await wait(1);

        // copy
        const rgba:Uint8Array = this.wasm.HEAPU8.slice(buffer, buffer+nbytes)
        rc = (rc == 0)? this.wasm.HEAP32[rc_ptr >> 2]! : rc;

        this.wasm._free(buffer);
        this.wasm._free(rc_ptr);
        delete this.#read_file_callback_table[handle];

        if(rc != 0)
            return new Error(`Reading tiff file failed. rc = ${rc}`)
        return {data:rgba, width:src_width, height:src_height};
    }

    async image_read_patch_and_encode(
        file:       File, 
        src_x:      number, 
        src_y:      number,
        src_width:  number,
        src_height: number,
        dst_width:  number,
        dst_height: number,
        lossless:   boolean,
    ): Promise<File|Error> {
        const handle:number = this.#handle_counter++;
        this.#read_file_callback_table[handle] = file;

        const output_buffer_pp:pointer = this.wasm._malloc(4);
        const output_size_p:pointer = this.wasm._malloc(8);
        const rc_ptr:pointer = this.wasm._malloc(4)
        this.wasm.HEAP32[rc_ptr >> 2] = 777;
        
        let output_buffer_p:pointer|undefined;

        let rc:number = 777;
        try {
            rc = this.wasm._image_read_patch_and_encode(
                file.size, 
                this.#read_file_callback_ptr, 
                handle, 
                src_x,
                src_y,
                src_width,
                src_height,
                dst_width,
                dst_height,
                lossless,
                output_buffer_pp, 
                output_size_p,
                rc_ptr
            )
            // NOTE: the wasm function above returns before the execution is 
            // finished because of async issues, so currently polling until done
            while(this.wasm.Asyncify.currData != null)
                await wait(1);
        
            
            rc = (rc == 0)? this.wasm.HEAP32[rc_ptr >> 2]! : rc;
            if(rc != 0)
                return new Error(`Reading tiff file failed. rc = ${rc}`)
            
            output_buffer_p = this.wasm.HEAP32[output_buffer_pp >> 2]!;
            const output_size:number = Number(this.wasm.HEAP64[output_size_p >> 3]);
            const encoded_image_data:Uint8Array = 
                this.wasm.HEAPU8.slice(output_buffer_p, output_buffer_p+output_size)
            
            // @ts-ignore typescript is annoying
            return new File([encoded_image_data], file.name, {type:file.type});
        } catch(e) {
            return e as Error;
        } finally {
        
            this.wasm._free(output_buffer_pp);
            this.wasm._free(output_size_p);
            this.wasm._free(rc_ptr);
            delete this.#read_file_callback_table[handle];

            if(output_buffer_p != undefined) 
                this.wasm._free_output_buffer(output_buffer_p);
            
        }
    }




    #handle_counter = 0;

    #read_file_callback_ptr:pointer;
    #read_file_callback_table: Record<number, File> = {};

    /** Called by WASM to read a required portion of a file */
    #read_file_callback = (
        handle: number,
        dstbuf: pointer,
        start:  bigint,
        size:   bigint,
    ): unknown  => {
        return this.wasm.Asyncify.handleAsync( async () => {
            const file:File|undefined = 
                this.#read_file_callback_table[handle];
            if(!file)
                return -1;
            
            const slice_u8:Uint8Array = new Uint8Array(
                await file.slice(Number(start), Number(start+size)).arrayBuffer()
            )
            this.wasm.HEAPU8.set(slice_u8, dstbuf);
            return 0;
        })
    }
}



export const initialize:typeof Iinitialize = async () => {
    const wasm:BigImageWASM = 
        // deno-lint-ignore no-explicit-any
        await (await import('../build-wasm/big-image.js')).default() as any;

    return new BigImage(wasm);
}



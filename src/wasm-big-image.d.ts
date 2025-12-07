



export type ImageSize = {width:number, height:number}
export type Image = {data:Uint8Array} & ImageSize;



export declare class BigImage {
    private constructor();

    /** Read image size from a JPEG/PNG/TIFF file without reading the full file */
    image_get_size(file:File): Promise<ImageSize|Error>;

    /** Read a patch from a JPEG/PNG/TIFF file, resize to the specified size
     *  [dst_width x dst_height] (via nearest neighbor interpolation) and return
     *  the result as a jpeg of png encoded `File` object. */
    image_read_patch_and_encode(
        file:       File, 
        src_x:      number, 
        src_y:      number,
        src_width:  number,
        src_height: number,
        dst_width:  number,
        dst_height: number,
        lossless:   boolean,
    ): Promise<File|Error>;

    
    image_read_patch(
        file:       File, 
        src_x:      number, 
        src_y:      number,
        src_width:  number,
        src_height: number,
        dst_width:  number,
        dst_height: number,
    ): Promise<Image|Error> ;
}


export declare function initialize(): Promise<BigImage>;



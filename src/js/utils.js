(function (Module) {
  Module.onRuntimeInitialized = () => {
    class BufferAllocator {
      static MAX_ALLOC_SIZE = 1024 * 1024 * 1024; // 1G
      static bufferSize = 0;
      static buffers = new Set();

      size = 0;
      ptr = 0;

      constructor(size) {
        this.size = size;
        if (this.size <= 0 && this.size > BufferAllocator.MAX_ALLOC_SIZE) {
          throw new TypeError(
            `Alloc buffer size should be 0 <= [size] < ${BufferAllocator.MAX_ALLOC_SIZE}!`
          );
        }
        this.ptr = Module._malloc(this.size);
        if (this.ptr <= 0) {
          throw new TypeError("Alloc buffer failed!");
        }
        BufferAllocator.buffers.add(this);
        BufferAllocator.bufferSize += this.size;
      }

      free() {
        if (this.ptr) {
          Module._free(this.ptr);
          BufferAllocator.buffers.delete(this);
          BufferAllocator.bufferSize -= this.size;
        }
      }

      get uint8Array() {
        return Module.HEAPU8.subarray(this.ptr, this.ptr + this.size);
      }

      get uint32Array() {
        return Module.HEAPU32.subarray(this.ptr, this.ptr + this.size);
      }

      get int8Array() {
        return Module.HEAP8.subarray(this.ptr, this.ptr + this.size);
      }

      get float32Array() {
        return Module.HEAPF32.subarray(this.ptr, this.ptr + this.size);
      }
    }

    Module.runCPPCallbackByAllocBuffer = (err, jsBuffer, seq) => {
      const cppBuffer = new BufferAllocator(jsBuffer.byteLength);
      cppBuffer.uint8Array.set(jsBuffer);
      Module.runCPPCallback(err, cppBuffer.ptr, cppBuffer.size, seq);
      // cppBuffer.free();
    };
  };
})(Module);

// Block, basic unit of all sub sections of the memory space.
//
// Each block will be placed in the large memory space, with an instance
// placed at the start of the block, with "size" bytes available directly
// after in memory. Member "size" represents this data space size, so the
// block actually takes up size + BlockInfoSize in MemSys data block.
class Block {
public:
    friend class MemSys;

    size_t dataSize() const { return size; }
    size_t totalSize()  const { return BlockInfoSize + size; }
    byte_t * data() { return (byte_t *)this + BlockInfoSize; }

private:
    size_t size = 0; // this is data size, not total size!
    Block * prev = nullptr;
    Block * next = nullptr;
    Type type = TYPE_FREE;

    // "info" could maybe be a magic string (for safety checks). or maybe additional info.
    // 8-byte allignment on current machine is forcing this Block to always be 32 bytes anyway
    // so this is here to make that explicit. We could even take more space from type, which
    // could easily be only 1 or 2 bytes.
    byte_t info[4];

    Block() {}

    // split block A into block A (with requested data size) and block B with what remains.
    Block * split(size_t blockANewSize) {
        // block a (this) is not big enough.
        // equal data size also rejected because new block would have
        // 0 bytes for data.
        if (dataSize() <= blockANewSize + BlockInfoSize) return nullptr;

        // where to put the new block?
        byte_t * newLoc = data() + blockANewSize;
        // write block info into data space with defaults
        Block * b = new (newLoc) Block();
        // block b gets remaining space
        b->size = dataSize() - BlockInfoSize - blockANewSize;

        // set this size
        size = blockANewSize;

        // link up
        b->next = next;
        b->prev = this;
        next = b;

        return b;
    }

    // merge this block with next block IF both are free
    Block * mergeWithNext() {
        if (type != TYPE_FREE || !next || next->type != TYPE_FREE) return nullptr;
        size += next->totalSize();
        next = next->next;
        return this;
    }
};
constexpr static size_t BlockInfoSize = sizeof(Block);

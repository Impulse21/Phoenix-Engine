
namespace phx
{
    struct AssetPackFile
    {
        struct AssetInfo
        {
            uint64_t PackedOffset;
            uint64_t PackedSize;
            uint16_t Type;
            uint16_t Flag;
        }

        struct SceneInfo
        {
            uint64_t PackedOffset;
            uint64_t PackedSize;
            uint16_t Flags;
        }

        struct IndexTable
        {
            uint32_t
        }

        struct FileHeader
        {
            const char Id[4] = { 'P', 'X', "A", "P"};
            uint32_t Version = 0;
            uint64_t BuildVersion = 0; // date/time foramt (<year><month><date><time>)
        }

        FileHeader Header = {};
        IndexTable Indexl = {};
    }
}
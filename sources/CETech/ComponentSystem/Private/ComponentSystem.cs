using System.Collections.Generic;
using System.Yaml;
using CETech.Develop;
using MsgPack;

namespace CETech
{
    public partial class ComponentSystem
    {
        private static readonly Dictionary<long, Spawner> _spawnerMap = new Dictionary<long, Spawner>();
        private static readonly Dictionary<long, Compiler> _compoilerMap = new Dictionary<long, Compiler>();

        public static void RegisterCompilerImpl(long type, Compiler compiler)
        {
            _compoilerMap[type] = compiler;
        }

        public static void CompileImpl(long type, YamlMapping body, ConsoleServer.ResponsePacker packer)
        {
            _compoilerMap[type](body, packer);
        }

        public static void RegisterSpawnerCompilerImpl(long type, Spawner spawner)
        {
            _spawnerMap[type] = spawner;
        }

        public static void SpawnImpl(int world, long type, int[] ent_ids, int[] ents_parent,
            MessagePackObjectDictionary[] data)
        {
            _spawnerMap[type](world, ent_ids, ents_parent, data);
        }
    }
}
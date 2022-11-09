using glTFLoader;
using glTFLoader.Schema;
using ImageMagick;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading.Tasks;

using ImageMagick;

namespace SceneConverter
{
    enum TextureUsage
    {
        None = 0,
        Abledo,
        Material,
        Emissive,
        Normal
    }

    internal class SceneConverterApp
    {
        static void Main(string[] args)
        {
            if (args.Length != 1)
            {
                Console.WriteLine($"Expecting input Path");
                return;
            }

            string input = args.FirstOrDefault();

            if (!File.Exists(input))
            {
                Console.WriteLine($"Unable to locate path {input}");
            }

            Gltf loadedModel = Interface.LoadModel(input);
            if (loadedModel == null)
            {
                Console.WriteLine($"Failed to load GLTF model {input}");
            }

            TextureUsage[] textureUsages = new TextureUsage[loadedModel.Textures.Length];

            // Collect a list of textures by usage
            foreach (Material mtl in loadedModel.Materials)
            {
                UpdateUsage(mtl.PbrMetallicRoughness.BaseColorTexture.Index, textureUsages, TextureUsage.Abledo);
                UpdateUsage(mtl.PbrMetallicRoughness.MetallicRoughnessTexture.Index, textureUsages, TextureUsage.Material);
                UpdateUsage(mtl.NormalTexture.Index, textureUsages, TextureUsage.Normal);

            }

            IList<Task> exportTasks = new List<Task>();
            IList<Image> newImages = loadedModel.Images.ToList();

            // Convert Textures
            foreach (Texture t in loadedModel.Textures)
            {
                int? sourceIndex = t.Source;
                if (sourceIndex == null)
                {
                    continue;
                }

                Image image = loadedModel.Images[sourceIndex.Value];
                string imageUri = image.Uri;
                string ddsImageUri = Path.Combine("texturesDDS", $"{Path.GetFileNameWithoutExtension(imageUri)}.dds");

                IMagickImage f = new IMagickImageFactory
                // Disbatch convert
                exportTasks.Add(
                    Task.Run(() =>
                    {
                        using (var image = new IMagickFactory.
                        {
                            // Save frame as jpg
                            image.Write(SampleFiles.OutputDirectory + "Snakeware.jpg");
                        }
                    }));

            }
        }

        static void UpdateUsage(int iTex, TextureUsage[] usageArray, TextureUsage usage)
        {
            if (usageArray[iTex] == TextureUsage.None)
            {
                usageArray[iTex] = usage;
            }

            Debug.Assert(usageArray[iTex] == usage);
        }
    }
}

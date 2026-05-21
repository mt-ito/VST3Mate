#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>

/**
 * Assets/sprites/ の PNG を juce_add_binary_data で埋め込んだものを
 * juce::Image の配列として返すユーティリティ。
 *
 * ファイル命名規則:
 *   idle フレーム    : idle_000.png, idle_001.png, ...
 *   clicked フレーム : clicked_000.png, clicked_001.png, ...
 *
 * BinaryData 変数名 : idle_000_png, idle_001_png, ...
 *                     clicked_000_png, clicked_001_png, ...
 */
class BinarySpriteLoader
{
public:
    static std::vector<juce::Image> loadIdleFrames()
    {
        return loadByKeyword("idle_");
    }

    static std::vector<juce::Image> loadClickedFrames()
    {
        return loadByKeyword("clicked_");
    }

    /**
     * BinaryData::namedResourceList をスキャンして keyword を含むリソースを
     * 名前順にロードして返す。
     */
    static std::vector<juce::Image> loadByKeyword(const juce::String& keyword)
    {
        std::vector<std::pair<juce::String, juce::Image>> named;

        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            juce::String name(BinaryData::namedResourceList[i]);
            if (name.startsWith(keyword))
            {
                int sz = 0;
                const char* data = BinaryData::getNamedResource(name.toRawUTF8(), sz);
                if (data && sz > 0)
                {
                    auto img = juce::ImageFileFormat::loadFrom(data, (size_t)sz);
                    if (img.isValid())
                        named.push_back({ name, std::move(img) });
                }
            }
        }

        std::sort(named.begin(), named.end(),
                  [](const auto& a, const auto& b){ return a.first < b.first; });

        std::vector<juce::Image> result;
        result.reserve(named.size());
        for (auto& p : named) result.push_back(std::move(p.second));
        return result;
    }
};

//
//  SS5Platform.cpp
//
#include "SS5PlayerPlatform.h"
#include "../include/texture.h"
#include "../include/quad.h"

/**
* 各プラットフォームに合わせて処理を作成してください
* DXライブラリ用に作成されています。
*/
#include <iostream>
#include "../include/sprite.h"

extern Sprite sprite;

namespace ss
{
    /**
    * ファイル読み込み
    */
    unsigned char* SSFileOpen(const char* pszFileName, const char* pszMode, unsigned long * pSize)
    {
        unsigned char * pBuffer = NULL;
        SS_ASSERT2(pszFileName != NULL && pSize != NULL && pszMode != NULL, "Invalid parameters.");
        *pSize = 0;
        do
        {
            // read the file from hardware
            FILE *fp = fopen(pszFileName, pszMode);
            SS_BREAK_IF(!fp);

            fseek(fp, 0, SEEK_END);
            *pSize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            pBuffer = new unsigned char[*pSize];
            *pSize = fread(pBuffer, sizeof(unsigned char), *pSize, fp);
            fclose(fp);
        } while (0);
        if (!pBuffer)
        {
            std::string msg = "Get data from file(";
            msg.append(pszFileName).append(") failed!");
            SSLOG("%s", msg.c_str());
        }
        return pBuffer;
    }

    /**
    * テクスチャの読み込み
    */
    long SSTextureLoad(const char* pszFileName, SsTexWrapMode::_enum  wrapmode, SsTexFilterMode::_enum filtermode)
    {
        /**
        * テクスチャ管理用のユニークな値を返してください。
        * テクスチャの管理はゲーム側で行う形になります。
        * テクスチャにアクセスするハンドルや、テクスチャを割り当てたバッファ番号等になります。
        *
        * プレイヤーはここで返した値とパーツのステータスを引数に描画を行います。
        */
        static long rc = 0;
        if (sprite.textures.size() <= rc)
            sprite.textures.resize(sprite.textures.size() * 2);
        sprite.textures[rc++] = new Texture(pszFileName);

        //SpriteStudioで設定されたテクスチャ設定を反映させるための分岐です。
        switch (wrapmode)
        {
        case SsTexWrapMode::clamp:    //クランプ
            break;
        case SsTexWrapMode::repeat:    //リピート
            break;
        case SsTexWrapMode::mirror:    //ミラー
            break;
        }
        switch (filtermode)
        {
        case SsTexFilterMode::nearlest:    //ニアレストネイバー
            break;
        case SsTexFilterMode::linear:    //リニア、バイリニア
            break;
        }

        return rc;
    }

    /**
    * テクスチャの解放
    */
    bool SSTextureRelese(long handle)
    {
        /// 解放後も同じ番号で何度も解放処理が呼ばれるので、例外が出ないように作成してください。
        bool rc = true;
        if (handle == -1)
        {
            rc = false;
        }
        delete sprite.textures[handle - 1];
        sprite.textures[handle - 1] = nullptr;
        return rc;
    }

    /**
    * テクスチャのサイズを取得
    * テクスチャのUVを設定するのに使用します。
    */
    bool SSGetTextureSize(long handle, int &w, int &h)
    {
        w = sprite.textures[handle - 1]->width;
        h = sprite.textures[handle - 1]->height;
        return true;
    }
    /**
    * スプライトの表示
    */
    void SSDrawSprite(State state)
    {
        if (state.blendfunc != BLEND_MIX && state.blendfunc != BLEND_ADD)
            std::cout << state.cellIndex << ": " << state.blendfunc << "," << state.colorBlendFunc << "," << state.colorBlendType << std::endl;
        sprite.shader.setInt("u_BlendType", state.blendfunc);
        if (state.blendfunc == BLEND_MIX)
            glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        else if (state.blendfunc == BLEND_ADD)
            glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        sprite.shader.setTexture2D("u_Texture", sprite.textures[state.texture.handle - 1]->id);
        sprite.shader.setBool("u_UseTexture", sprite.textures[state.texture.handle - 1]->loaded);

        sprite.shader.setVec4("u_Quad[0].vertex", glm::make_mat4(state.mat) * glm::vec4(*(glm::vec3*)(&state.quad.tl.vertices), 1));
        sprite.shader.setVec4("u_Quad[1].vertex", glm::make_mat4(state.mat) * glm::vec4(*(glm::vec3*)(&state.quad.bl.vertices), 1));
        sprite.shader.setVec4("u_Quad[2].vertex", glm::make_mat4(state.mat) * glm::vec4(*(glm::vec3*)(&state.quad.tr.vertices), 1));
        sprite.shader.setVec4("u_Quad[3].vertex", glm::make_mat4(state.mat) * glm::vec4(*(glm::vec3*)(&state.quad.br.vertices), 1));

        sprite.shader.setVec2("u_Quad[0].uv", state.quad.tl.texCoords.u, state.quad.tl.texCoords.v);
        sprite.shader.setVec2("u_Quad[1].uv", state.quad.bl.texCoords.u, state.quad.bl.texCoords.v);
        sprite.shader.setVec2("u_Quad[2].uv", state.quad.tr.texCoords.u, state.quad.tr.texCoords.v);
        sprite.shader.setVec2("u_Quad[3].uv", state.quad.br.texCoords.u, state.quad.br.texCoords.v);

        sprite.shader.setVec4("u_Quad[0].color", glm::vec4(*(glm::u8vec4*)(&state.quad.tl.colors)) / 255.f * float(state.opacity) / 255.f);
        sprite.shader.setVec4("u_Quad[1].color", glm::vec4(*(glm::u8vec4*)(&state.quad.bl.colors)) / 255.f * float(state.opacity) / 255.f);
        sprite.shader.setVec4("u_Quad[2].color", glm::vec4(*(glm::u8vec4*)(&state.quad.tr.colors)) / 255.f * float(state.opacity) / 255.f);
        sprite.shader.setVec4("u_Quad[3].color", glm::vec4(*(glm::u8vec4*)(&state.quad.br.colors)) / 255.f * float(state.opacity) / 255.f);

        sprite.render_quad();
    }

    /**
    * ユーザーデータの取得
    */
    void SSonUserData(Player *player, UserData *userData)
    {
        //ゲーム側へユーザーデータを設定する関数を呼び出してください。
    }

    /**
    * ユーザーデータの取得
    */
    void SSPlayEnd(Player *player)
    {
        //ゲーム側へアニメ再生終了を設定する関数を呼び出してください。
    }

    /**
    * windows用パスチェック
    */
    bool isAbsolutePath(const std::string& strPath)
    {
        std::string strPathAscii = strPath.c_str();
        if (strPathAscii.length() > 2
            && ((strPathAscii[0] >= 'a' && strPathAscii[0] <= 'z') || (strPathAscii[0] >= 'A' && strPathAscii[0] <= 'Z'))
            && strPathAscii[1] == ':')
        {
            return true;
        }
        return false;
    }

};

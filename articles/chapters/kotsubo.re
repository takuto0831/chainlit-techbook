
= AIが今何をしているか？で不安にならないようにしよう！

//lead{
LLM がツールを使って複雑な処理をこなすようになった今、応答が返るまでの「待ち時間」はユーザー体験の一つの課題です。@<chap>{uchiyama} では開発者目線での LLM の観察・デバッグを扱いましたが、本章では@<b>{ユーザー目線}に立ち、待ち時間の不安をどう解消するかに焦点を当てます。Chainlit を使い始めていてユーザー体験をさらに良くしたい方に向けて、@<code>{cl.Step} を起点に、Markdown による構造化・@<code>{cl.TaskList} によるタスクの表示・@<code>{cl.Plotly} による結果の可視化・@<code>{asyncio.gather} を活用したメッセージの並行表示という4つのアプローチを紹介します。Chainlit を題材にしていますが、背景にある考え方は他のフレームワークにも通じるものです。
//}
//pagebreak

== 背景

LLM の精度向上とツールによる機能拡張が進むにつれ、1回の応答にかかる時間が長くなる傾向があります。処理の過程を何も表示しないと、ユーザーは「本当に動いているのか？」という不安を抱きます。さらに、途中経過が何も見えないまま長い時間待った末に、意図とまったく異なる結果が返ってきた場合、待っていた時間そのものが無駄になってしまいます（@<img>{no_step}）。途中経過が見えていれば、方向がずれた時点で早めに気づいて修正できたかもしれません。本章では、処理の過程をわかりやすく表示する工夫と、待ち時間を有効活用する工夫を紹介します。

//image[no_step][思考の過程を表示しない例][scale=0.8]{
//}

== 本章でわかること

 * @<code>{cl.Step} を使った処理過程の可視化
 * Step の出力を Markdown でリッチに表示する方法
 * @<code>{cl.TaskList} による全体進捗のサイドバー表示
 * @<code>{cl.Plotly} によるインタラクティブなチャート表示
 * @<code>{asyncio.gather} を使った並行処理で待ち時間を有効活用する方法

== 本章で利用するアプリケーション

本章では、複数のトピックを検索するリサーチアプリケーション @<fn>{support} を例に、Chainlit の @<code>{cl.Step} 機能を中心に説明します。このアプリケーションは、ユーザーがブラウザから質問を送ると、OpenAI の @<code>{web_search_preview} ツールを使って複数のトピックをウェブ検索し、結果を集約して回答を返すチャットボットです。処理の流れは次のとおりです。

//footnote[support][本章のソースコードは以下のリポジトリの @<code>{ch05-progress} ディレクトリから参照できます。@<href>{https://github.com/statditto/chainlit-techbook-support}]

 1. ユーザーのクエリから調査トピックを3つ生成する
 2. 各トピックについてウェブ検索を行い、ソースごとに要約・信頼度を付与する
 3. 全調査結果を集約して最終回答をストリーミング表示する

1つのメッセージに対して複数回の LLM 呼び出しとウェブ検索が走るため、応答までに数十秒かかることもあります。この待ち時間をどう扱うかが本章のテーマです。

== リサーチアプリケーションの実装

=== @<code>{cl.Step} とは？

@<code>{cl.Step} は処理の「中間状態」を UI に表示するための仕組みです。基本的な使い方や @<code>{type} パラメータの種類については @<chap>{uchiyama} を参照してください。

本章のリサーチアプリでは、@<code>{type} パラメータを次のように使い分けています。

 * @<code>{"tool"} --- ウェブ検索全体やトピックごとの調査ステップに使用
 * @<code>{"llm"} --- プロンプトの送信や回答の生成など、モデルへの呼び出しに使用
 * @<code>{"retrieval"} --- ウェブ検索の各ソースへのアクセスに使用

また、@<code>{async with} をネストすることで階層構造を表現できます。@<chap>{uchiyama} では @<code>{parent_id} を指定する方法が紹介されていますが、本章ではコンテキストマネージャのネストで同じことを実現しています。

//pagebreak

//emlist[入れ子にした@<code>{cl.Step}の使い方][python]{
async with cl.Step(name="親", type="tool") as parent:
    async with cl.Step(name="子", type="retrieval") as child:
        child.output = "子の結果"
    parent.output = "親の結果"
//}

=== ベース実装

本章のアプリケーションでは、次のような3段階の入れ子構造で @<code>{cl.Step} を構成しています。

//emlist[リサーチアプリケーションのベース実装][python]{
@cl.on_message
async def main(message: cl.Message) -> None:
    query = message.content
    all_findings: list[str] = []

    async with cl.Step(
            name="ウェブを調査しています", type="tool"
        ) as root_step:
        root_step.input = query

        topics = await generate_topics(query)

        for topic in topics:
            async with cl.Step(
                name=f"「{topic}」を調査中", type="tool"
            ) as topic_step:
                topic_step.input = f"「{topic}」の観点で調査"

                sites = await research_topic(query, topic)

                for site in sites:
                    async with cl.Step(
                        name=f"{site['name']}", type="retrieval"
                    ) as site_step:
                        site_step.input = site["url"]
                        site_step.output = site["summary"]

                    all_findings.append(
                        f"**[{topic}｜{site['name']}]** {site['summary']}"
                    )

                topic_step.output = f"{len(sites)} 件のソースを確認しました"

        root_step.output = f"合計{len(all_findings)} 件のソースを調査しました"
//}

@<code>{generate_topics} は LLM にユーザーのクエリを渡し、調査すべき観点を複数の文字列（@<code>{topic}）として返す関数です。たとえば「Chainlit とは何か」というクエリに対して @<code>{["基本概念", "主な機能", "活用事例"]} のようなリストが返ります。各 @<code>{topic} に対してトピック Step が1つ生成され、その中でウェブ検索が実行されます。

//emlist[@<code>{generate_topics}で利用しているプロンプト][python]{
messages=[
    {
        "role": "system",
        "content": (
            "あなたはリサーチアシスタントです。JSONのみを返してください。"
        ),
    },
    {
        "role": "user",
        "content": (
            f"「{query}」を調査するための主要なトピックを3つ挙げてください。"
            '{"topics": ["トピック1", "トピック2", "トピック3"]}'
            " の形式で返してください。"
        ),
    },
],
//}

@<code>{cl.Step} のツリー構造は次のようになっています。

//emlist[@<code>{cl.Step} のツリー構造]{
▼ ウェブを調査          ← root_step（type="tool"）
  ▼ 「基本概念」を調査      ← topic_step（type="tool"）
    ▼ Wikipedia              ← site_step（type="retrieval"）
    ▼ 公式ドキュメント         ← site_step（type="retrieval"）
  ▼ 「応用例」を調査        ← topic_step（type="tool"）
    ...
[最終回答]          ← Step の外でストリーミング表示
//}

全トピックの調査が完了した後、集約結果を最終回答としてストリーミング表示します。@<code>{cl.Message} の @<code>{stream_token} メソッドを使うことで、LLM が生成するテキストを逐次的に画面へ反映できます。

//emlist[最終回答のストリーミング表示][python]{
answer_msg = cl.Message(content="")
await answer_msg.send()

stream = await client.chat.completions.create(
    model="gpt-4o-mini",
    stream=True,
    messages=[...],
)
async for chunk in stream:
    delta = chunk.choices[0].delta.content
    if delta:
        await answer_msg.stream_token(delta)

await answer_msg.update()
//}

実行すると @<img>{ui_base} のように、処理の階層が折りたたみ UI として表示されます。「何かが動いている」ことが視覚的にわかるようになりました。

//image[ui_base][基本的な Step 利用時の表示例][scale=0.8]{
//}

== Markdown によるステップ出力の構造化

ベース実装では、トピック Step の @<code>{output} に「2 件のソースを確認しました」という単純なテキストを設定していました。ここを Markdown のテーブル形式に変更します。

//emlist[Markdown テーブルを生成するフォーマッター][python]{
def fmt_topic_output(_topic: str, sites: list[dict]) -> str:
    rows = "\n".join(
        f"| [{s['name']}]({s['url']}) | `{urlparse(s['url']).netloc}`"
        f" | {s['reliability']} |"
        for s in sites
    )
    return (
        f"| ソース | ドメイン | 信頼度 |\n"
        f"|--------|----------|--------|\n"
        f"{rows}\n\n"
        f"> **{len(sites)} 件**のソースを確認しました"
    )
//}

@<code>{topic_step.output = fmt_topic_output(topic, sites)} と差し替えるだけで適用できます（@<img>{ui_markdown2}）。なお信頼度は、LLM に対して★の数（1〜5個）で返すよう指示することで付与しています。

//image[ui_markdown2][Markdown フォーマットを導入した Step 表示の例][scale=0.8]{
//}

ここで @<code>{fmt_topic_output} に渡す変数 @<code>{sites} は、次に紹介する @<code>{research_topic} 関数の戻り値です。@<code>{research_topic} がウェブ検索と要約を行い構造化データ（ソース名・URL・要約・信頼度）を返し、@<code>{fmt_topic_output} がそれを Markdown テーブルに整形して Step の @<code>{output} に設定するという役割分担になっています。

@<code>{research_topic} は、OpenAI の @<code>{web_search_preview} ツールでウェブ検索を行い、取得したテキストと引用 URL を LLM に渡して構造化する関数です。内部では2回の API 呼び出しを行っています。1回目の @<code>{responses.create} でウェブ検索と要約を実行し、2回目の @<code>{chat.completions.create} で検索結果をソース名・URL・要約・信頼度の JSON 形式に整形します。以下は2回目の呼び出しで使用しているプロンプトです。

//emlist[@<code>{research_topic}での文章の要約と信頼度を返すプロンプト][python]{
{
    "role": "user",
    "content": (
        f"以下の調査結果と参考URL一覧をもとに情報ソースを最大3件まとめてください。\n\n"
        f"調査結果:\n{output_text}\n\n"
        f"参考URL:\n{citations_text}\n\n"
        '{"sources": [{"name": "ソース名", "url": "実際のURL",'
        ' "summary": "そのソースの要約", "reliability": "★～★★★★★"}]}'
        " の形式で返してください。"
        "urlは参考URL一覧にある実際のURLをそのまま使ってください。"
        "reliabilityは情報源の信頼度を★の数（1〜5個）で表してください。"
    ),
},
//}

この変更により、次のようなメリットがあります。

 * @<b>{1クリックで全ソースを表示}：サイト Step を個別に展開しなくても、トピック Step の output 一覧だけで調査先と信頼度が把握できます
 * @<b>{ソース名がリンク化}：クリックで実際の URL に飛べます
 * @<b>{ドメインをコードブロックで表示}：@<code>{`ja.wikipedia.org`} の形式で情報源の出所が一目で判断できます
 * @<b>{信頼度が視覚的}：★の数で信頼度の高低をすばやく把握できます

== TaskList によるサイドバー進捗表示

Step のツリー表示は処理の「詳細」を確認するためのものです。トピック数が増えると、どこまで処理が終わっているかが一目でわかりません。@<code>{cl.TaskList} と @<code>{cl.Task} を追加し、サイドバーに全体進捗を表示します。

//pagebreak

//emlist[TaskList と Task の追加][python]{
@cl.on_message
async def main(message: cl.Message) -> None:
    task_list = cl.TaskList()
    task_list.status = "調査中..."
    await task_list.send()

    # ...（Step のネスト構造はそのまま）

    for topic in topics:
        task = cl.Task(title=f"「{topic}」を調査中",
                       status=cl.TaskStatus.RUNNING)
        await task_list.add_task(task)
        await task_list.update()

        async with cl.Step(...) as topic_step:
            # ... 処理 ...
            pass

        task.status = cl.TaskStatus.DONE
        await task_list.update()
//}

各トピックの処理開始時に @<code>{RUNNING} で追加し、完了時に @<code>{DONE} へ更新することで、リアルタイムな進捗が反映されます。

@<img>{ui_tasklist} は1つ目のトピック処理中の状態、@<img>{ui_tasklist2} は1つ目のトピックが完了した後の状態です。

//image[ui_tasklist][タスク一覧の表示例（1つ目のタスクの処理開始中）][scale=0.8]{
//}

//image[ui_tasklist2][タスク一覧の表示例（1つ目のタスクの処理終了後）][scale=0.8]{
//}

この変更により、次のようなメリットがあります。

 * @<b>{スクロールせずに全体進捗を可視化}：サイドバーは常時表示されるため、ユーザーが Step ツリーをスクロールしていても進捗を確認できます
 * @<b>{「今どのトピックを処理中か」が明確}：実行中のタスクがハイライト表示されます

== Plotly ヒートマップによる信頼度の可視化

全トピックの調査完了後に、Plotly のインタラクティブなヒートマップを @<code>{cl.Plotly} で表示します。前の節で表示していた信頼度（★の数）を数値に変換し、「トピック × ソース番号」のマトリクスとして色で表現します。

//emlist[Plotly ヒートマップの表示][python]{
all_sites: list[dict] = []  # チャート用：topic 付きで全ソースを蓄積

# ... 省略 ...

if all_sites:
    fig = make_reliability_chart(all_sites)
    await cl.Message(
        content="**情報ソース 信頼度チャート**",
        elements=[
            cl.Plotly(name="reliability_chart", figure=fig, display="inline")
        ],
    ).send()
//}

@<code>{make_reliability_chart} では @<code>{plotly.graph_objects.Heatmap} を使い、★の個数をセルの色として表現しています。セルにはソース名と信頼度が重ねて表示されます（@<img>{ui_chart}）。

//image[ui_chart][ヒートマップによる信頼度の可視化][scale=0.8]{
//}

この変更により、次のようなメリットがあります。

 * @<b>{どのトピックのどのソースが高品質かを視覚的に把握}：テキストで並んだ★と異なり、色による比較は直感的です
 * @<b>{インタラクティブ}：Plotly のチャートはホバーでツールチップが表示され、ズームや拡大も可能です
 * @<b>{全体感の把握}：複数トピックにわたるソースの信頼度分布をまとめて確認できます


== @<code>{asyncio.gather}を活用したメッセージの並行表示

@<code>{asyncio.gather} を使って、「トピック生成」と「豆知識生成」を並行実行します。トピック生成が完了した時点で豆知識も揃っており、リサーチ開始と同時にユーザーへ関連情報を表示できます。

//emlist[並行実行と豆知識の即時表示][python]{
@cl.on_message
async def main(message: cl.Message) -> None:
    query = message.content

    # トピック生成と豆知識生成を並行実行
    topics, trivia = await asyncio.gather(
        generate_topics(query),
        generate_trivia(query),
    )

    # 豆知識を即時表示
    await cl.Message(
        content=(
            f"{trivia}\n\n"
            f"---\n"
            f"*リサーチを開始しました。結果が出るまでしばらくお待ちください...*"
        )
    ).send()

    # 以降は通常どおりリサーチを実行
    async with cl.Step(...):
        ...
//}

@<code>{generate_topics} は「リサーチアプリケーションの実装」節で紹介した関数です。@<code>{generate_trivia} はそれと並行して実行する新しい関数で、クエリに関連する豆知識を1件返します。

//emlist[@<code>{generate_trivia} で利用しているプロンプト][python]{
messages=[
    {
        "role": "system",
        "content": (
            "あなたは博識なアシスタントです。"
            "与えられたテーマに関連する、知っていると少し得する面白い豆知識を1つ、"
            "日本語で3〜5文程度で教えてください。"
            "「💡 豆知識：」で始めてください。"
        ),
    },
    {
        "role": "user",
        "content": f"「{query}」に関連する豆知識を1つ教えてください。",
    },
],
//}

@<code>{generate_topics} と並行して実行されるため、処理を待っている間に豆知識を読むことができます（@<img>{ui_trivia}）。

//image[ui_trivia][豆知識の表示例][scale=0.8]{
//}

この変更により、次のようなメリットがあります。

 * @<b>{待ち時間を退屈させない}：ユーザーがリサーチ結果を待つ間、テーマに関連した読み物を提供できます
 * @<b>{追加のレイテンシがほぼゼロ}：豆知識生成はトピック生成と並行して走るため、シーケンシャルな場合と比べて待ち時間はほとんど増えません
 * @<b>{「処理が始まった」という安心感}：最初に何かが表示されることで、ユーザーはアプリが応答したと認識できます

== まとめ

本章では、LLM アプリケーションの待ち時間に対するユーザー体験を向上させる4つのアプローチを紹介しました。以下に各内容とメリットをまとめます。

//table[summary][ベース実装からの変更内容とメリットの一覧]{
変更内容	主なメリット
----------------
@<code>{cl.Step} 出力のテーブル形式化	ソース・信頼度が展開不要で一覧できる
サイドバーに進捗リストを表示	全体の何件中何件が完了したか一目でわかる
信頼度ヒートマップを表示	ソース品質の比較が色で直感的にできる
豆知識の並行表示	待ち時間を有効活用し初動表示を早める
//}

4つのアプローチは「処理の過程をリアルタイムに見せる工夫（@<code>{cl.Step}・@<code>{cl.TaskList}）」「待ち時間そのものを有意義にする工夫（豆知識の並行表示）」「処理後の結果理解を助ける工夫（@<code>{cl.Plotly} によるヒートマップ）」という3つの方向性に整理できます。
本章では Chainlit の API を使って実装しましたが、「処理の階層と進捗をリアルタイムに伝える」「待ち時間に別のコンテンツを並行生成して表示する」といった設計パターンはフレームワークに依存しません。Streamlit や Gradio、あるいは独自のフロントエンドでも同様に適用できます。ぜひ皆さんのアプリにも取り入れてみてください！

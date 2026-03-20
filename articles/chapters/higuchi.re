
= ChainlitのUIを@<ruby>{癖, へき}に染める

//lead{
Chainlitは簡単にLLMとの会話アプリを作れるよう、シンプルで使いやすいUIが標準装備されています。
加えて、カスタムCSSやカスタムJavaScriptも反映できるため、凝ったデザインの適用も可能です。
この章では、これらの機能を使ってSF風のUIの実装を行います。

フロントエンドの経験が少ない方でも取り組める内容です。
普段使いのチャットUIを思い通りにカスタマイズしてみましょう。
//}

//pagebreak

== はじめに
生成AIが一般社会に急速に浸透し、我々の生活やワークスタイルは大きく変化しました。日々の献立から専門的な分野の調査分析まで、普段使う言葉で相談できる生成AIは、もはや日常になくてはならないものになっています。それはまるで、先人たちがSF作品で描いた世界のようです。

ゲームに絞っても、人工知能との関わりを描いた作品は数多く思い当たります。「ナビ」と呼ばれる人工知能を使ってインターネットウイルスを駆逐したり、2つのポータルと人間の知恵を駆使して暴走した人工知能と対峙したり。もしかしたら現実の生成AIに対し、単なるツールを超えた人格としての親近感を覚える方もいるのではないでしょうか。少なくとも私はそうです。

このような作品では、世界観に合わせて近未来的なUIが導入される傾向にあります。その鮮烈なネオンカラーと非自然的なポリゴンデザインの格好良さは脳裏に焼きついて離れません。現実世界に存在する生成AIサービスのUIデザインはシンプルなものが多いですが、あえてSF風に寄せてみたら、日々の業務がちょっと楽しくなるのではないか？そんな思いつきから、ChainlitのUI変更に挑戦することにしました。

== この記事の取扱範囲と資料について
=== 取扱範囲・取り扱わない範囲

この記事では@<b>{Chainlitに対するCSS, JavaScript(以下JS)の適用による要素の変更}を取り扱います。下記の項目は取り扱いません。

==== CSS, JSの詳しい解説

CSSやJSを使ってUIを変更していきますが、それぞれの基本的な記法の解説は省略します。
また、Chainlitの内部ではReactやTailwind CSSなどが採用されていますが、今回のカスタマイズではこれらを直接操作しないため、解説は行いません。

==== Chainlitのその他の要素の解説

UI変更の実装のために、ベースとなる簡単なアプリケーションを作成しました。具体的には下記の機能を持ちます。

 * ログイン機能
 * 送信されたメッセージをオウム返しする
 * 「@<code>{demo}」と入力すると@<code>{text}, @<code>{plotly}, @<code>{table}の@<code>{element}を返す

しかし、あくまでもこの記事の主題はUI変更のため、この機能の解説は省略します。

==={sample-code} サンプルコードとAppendixサイトについて

この記事ではUIのカラー変更やアニメーション適用を扱いますが、書籍版はモノクロ印刷です。
これらの内容の補足のために、Appendixサイト@<fn>{appendix-site}に参考資料を掲載しました。
合わせて、装飾後のコードもGitHubで公開しています@<fn>{finished-repo}。
必要に応じてご参照ください。

//footnote[appendix-site][Appendixサイト: @<href>{https://seahawk-tiger.github.io/chainlit-ui-appendix/}]
//footnote[finished-repo][変更後コードは以下のURLの @<code>{ch10-cyber-ui} ディレクトリから参照できます。 @<href>{https://github.com/statditto/chainlit-techbook-support}]

//image[seahawk-qr-01][サンプルコード・AppendixサイトのQRコード][scale=0.6]


== ChainlitのUI変更の仕様を知る

ChainlitはLLMとの会話アプリを簡単に実装することを主眼に作られています。フロントエンド開発の知識が乏しくてもアプリを構築できるよう、標準で使いやすいチャットUIが用意されています。これらはReact + Tailwind + Shadcn/uiの構成でChainlit内部に実装されており、通常のプロジェクト作成時にはフロントエンドのソースコードはプロジェクトディレクトリに生成されません。

ただし、簡単なデザイン変更はサポートされています。具体的には、@<code>{config.toml}に装飾用のファイル（CSS, JS, ロゴなど）のパスを書き込むことで、標準UIのスタイルを上書きできます。

整理すると、@<b>{HTMLは直接の編集が難しく、CSSとJSは適用が容易}という状況です。そのため、この記事では@<b>{標準UIをCSSで静的・動的に装飾しつつ、必要であればJSでHTMLの要素を編集する}という作戦で実装することにしました。

=={design} 実装するデザインとアニメーション

Figmaでスタイルガイド@<fn>{color-tokens}とデザイン案@<fn>{design-color}を作成しました。この案を元に実装していきます。厳密に再現するというよりは、実装難度に応じて多少の差異を許容することにします。
このうち、ユーザー・アシスタントの両方の吹き出しには短い時間で2回枠の色が明るくなるような演出を付けます（以降、明滅の演出と呼びます）。

//image[seahawk-design-prop-ml-combined][デザインサンプルとAppendixサイトのQRコード][scale=0.7]

また、今回の実装にあたり、円をモチーフにしたロゴを作成しました@<fn>{logo-file-url}。このロゴは、半径の異なる8種類の円から構成されています@<fn>{logo-desc}。演出として、このロゴの円を回転させます。

//image[seahawk-logo-full-bg-ml-combined][ロゴとAppendixサイトのQRコード][scale=0.6]


//footnote[color-tokens][スタイルガイド: @<href>{https://seahawk-tiger.github.io/chainlit-ui-appendix/pages/appendix1-color-and-font.html}]
//footnote[design-color][デザイン案: @<href>{https://seahawk-tiger.github.io/chainlit-ui-appendix/pages/appendix2-ui-design-proto.html}]
//footnote[logo-file-url][ロゴのサンプルファイルは@<hd>{sample-code}で紹介したGitHubで公開しています。]
//footnote[logo-desc][ロゴの構成について: @<href>{https://seahawk-tiger.github.io/chainlit-ui-appendix/pages/appendix3-logo-structure.html}]


== 変更していこう

==={change-logo} ロゴを変更する
手始めにロゴを変えていきましょう。このロゴはfaviconにも使われます。

まず、@<code>{.chainlit}ディレクトリ@<fn>{chainlit-dir}と同じ階層に@<code>{public}ディレクトリを作り、faviconとロゴに使うファイルを配置します。

//footnote[chainlit-dir][.chainlitディレクトリについての詳細は@<chap>{first_steps}を参照してください。]

//emlist[ディレクトリ構造]{
.chainlit
├── translations/ …省略
└── config.toml
...
public/ <- 作成
└── logo.svg <- 追加
※ その他のディレクトリは省略
//}

その後、@<code>{config.toml}を開き、配置したロゴファイルのパスを入力すれば完了です。
faviconにも@<code>{logo_file_url}で指定されたファイルが使われます。
ついでに、AIの返答メッセージの横に表示されるアイコンも同じロゴファイルにしておきましょう。

//pagebreak

//emlist[.chainlit/config.toml][toml]{
# Load assistant logo directly from URL.
logo_file_url = "/public/logo.svg"

# Load assistant avatar image directly from URL.
default_avatar_file_url = "/public/logo.svg"
//}

補足として、@<code>{config.toml}には他にもログインページの背景画像等の設定項目があります。
今回は設定しませんが、これらを使えば標準UIのデザインをさらに好みに近づけられます。
下記は対応するキーと記載例です。
//emlist[.chainlit/config.toml][toml]{
login_page_image = "/public/custom-background.jpg"
login_page_image_filter = "brightness-50 grayscale"
login_page_image_dark_filter = "contrast-200 blur-sm"
custom_meta_image_url = "(略)/logo/chainlit_banner.png"
//}

=== 見出しやボタンの表記を変える
@<code>{.chainlit}の配下に@<code>{translations}というディレクトリと多数のJSONが自動生成されます。これらは、ChainlitのUIで使用されるラベルやボタン、メニューなどのテキストの翻訳辞書です。各言語でファイルが分かれており、それぞれのファイルを編集することで、UIの表示文言を任意の言葉に変更できます。デフォルトでは「ログイン」「パスワード」など、日本語を使った表記になっているのですが、今回は見た目重視で英単語で表記されるように変更します。

//emlist[.chainlit/translations/ja.json][json]{
    "auth": {
        "login": {
            "title": "SYSTEM LOGIN",
            "form": {
                "email": {
                    "label": "USER ID",
                    "required": "USER ID IS REQUIRED",
                    "placeholder": "USER ID"
                },
                ...
            },
//}


ここまでで、ロゴと表記を変更できました。今の見た目はこのようになっています。左側のタイトルと右半分の背景に注目してください。
//image[seahawk-login-compare-ml][ロゴの適用前後比較][scale=1]


=== 静的要素を変更する

===={initial_settings} 初期設定と@<code>{custom_theme}について


ここからは、CSSを編集して静的要素を変更していきます。

ロゴと同様に、装飾に使うスタイルシートを@<code>{public}配下に配置します。もう一つ大事な要素として、@<code>{theme.json}というファイルも作っておきましょう。@<code>{theme.json}は、ChainlitのUIで使用されるカラーパレットやデザイントークンを定義するためのファイルで、アプリケーション全体の配色やUIテーマを統一できます。

//emlist[ディレクトリ構造]{
その他のディレクトリは省略
.chainlit
├── translations/ …省略
└── config.toml

public/
├── logo.svg
├── theme.json <- 新規作成
├── effects.js <- 新規作成
└── stylesheet.css <- 新規作成
//}

ファイルの作成後、@<code>{config.toml}を編集します。これもロゴの登録と同じ要領です。
注意点として、@<code>{custom_css} や @<code>{custom_js} は @<code>{.chainlit/config.toml}にコメント付きの設定例が記載されていますが、@<code>{custom_theme}の設定はテンプレートに含まれていない場合があります。その場合は、手動で@<code>{config.toml}に@<code>{custom_theme}の設定を記載してください。

また、JSファイルの登録も同じ要領です。後で使うのでここで一緒に作っておきましょう。

//emlist[.chainlit/config.toml][toml]{
custom_theme = "/public/theme.json"
custom_css = "/public/stylesheet.css"
custom_js = "/public/effects.js"
//}

その後、@<code>{theme.json}にあらかじめ設計したカラーや枠線のスタイルを記載します。この記法はShadcnのTheme記法に従います。下記に設定例を示しますが、詳細は章末のリンクを参照してください。
//emlist[public/theme.json][json]{
{
  "custom_fonts": [],
  "variables": {
    "light": {
      "--background": "0 0% 100%",
      "--foreground": "0 0% 5%",
      ...
    },
    "dark": {
      "--background": "222 47% 6%",
      "--foreground": "222 40% 96%",
      ...
    }
  }
}
//}

==== ログイン画面の変更


ここからは@<b>{セレクタを特定し、CSSやJSで書き換える}という作業を繰り返してデザインを実装していきます。
目標のデザインを実現する上で、ログイン画面内で大きな改造が必要な3つの項目を説明します。

 * 標準UIの背景のロゴを消し、入力欄を中央に配置する
 * ロゴを入力欄の上部に表示する
 * その他の要素の変更

===== 標準UIのロゴを消し、入力欄を中央に配置する @<br>{}
まず、ログイン画面の判定のために、下記のセレクタを利用します。
//emlist[public/stylesheet.css][css]{
body:has(input[type="password"]):has(button[type="submit"])
//}

続いて、標準UIの背景画像は下記のセレクタで指定できるので、@<code>{display: none}でまるごと消します。
//emlist[public/stylesheet.css][css]{
body:has(input[type="password"]):has(button[type="submit"])
  img.absolute.inset-0.h-full.w-full.object-cover {
  display: none !important;
}
//}

続いて、フォームを中央に配置する処理です。まず、rootとmainを画面幅いっぱいに広げます。その後、フォームを含むmainを中央寄せし、さらにその直下の子要素も中央寄せします。
//emlist[public/stylesheet.css][css]{
body:has(input[type="password"]):has(button[type="submit"]) #root,
body:has(input[type="password"]):has(button[type="submit"]) main{
  ...
  width: 100%;
  place-items: center;
  ...
}

body:has(input[type="password"]):has(button[type="submit"])
  main:has(form) {
  ...
  justify-items: center !important;
  align-items: center !important;
  ...
}

body:has(input[type="password"]):has(button[type="submit"])
  main:has(form) > :has(form) {
  ...
  place-items: center !important;
  ...
}
//}

===== ロゴを入力欄の上部に表示する @<br>{}
ロゴを画面の中心かつ入力欄の上に配置するため、@<code>{logo.svg}を参照している要素を探して、中央寄せします。

//emlist[public/stylesheet.css][css]{
body:has(input[type="password"]):has(button[type="submit"])
  :is(img.logo, img[src="/public/logo.svg"], img[src="/logo"]) {
  display: block !important;
  width: 180px;
  height: auto;
  margin: 0 auto 24px auto !important;
}
//}

===== その他の要素の変更 @<br>{}
残りのフォームの要素や見出し、ボタンを装飾します。下記は入力ボックスの例です。ログイン画面全体の@<code>{input}を指定しており、同じ要領で@<code>{button}も装飾しています。他の装飾の解説は割愛します。

//emlist[public/stylesheet.css][css]{
body:has(input[type="password"]):has(button[type="submit"]) input {
  width: 100%;
  height: 42px;
  background: hsl(var(--background));
  color: hsl(var(--foreground));
  border: 1px solid hsl(var(--border));
  ...
//}

CSS適用により、ログイン画面はこのような見た目になりました。デザイン案に近づいてきましたね。

//image[seahawk-customized-login-2-ml][装飾後のログイン画面][scale=0.6]

//pagebreak

==== トーク画面の変更

ログイン画面に比べて、トーク画面の変更はシンプルです。トーク画面の判定には下記のセレクタを利用します。

//emlist[public/stylesheet.css][css]{
body:not(:has(input[type="password"]):has(button[type="submit"]))
//}

トーク画面には入力ボックス・ユーザー側の吹き出し・アシスタント側の吹き出し（LLM側の返答）があり、それぞれのセレクタは下記の通りです。アシスタント側の吹き出しはユーザー側の吹き出しに該当するクラスの除外により区別しています。

//emlist[public/stylesheet.css][css]{
/* 入力ボックスのセレクタ */
body:not(:has(input[type="password"]):has(button[type="submit"]))
  #message-composer

/* ユーザー側の吹き出しのセレクタ */
body:not(:has(input[type="password"]):has(button[type="submit"]))
  div.px-5.py-2\.5.relative.bg-accent.rounded-3xl
  .max-w-\[70\%\].flex-grow-0

/* アシスタント側の吹き出しのセレクタ */
body:not(:has(input[type="password"]):has(button[type="submit"]))
  .message-content:not(
    .px-5.py-2\.5.relative.bg-accent.rounded-3xl
    .max-w-\[70\%\].flex-grow-0
    .message-content
  )
//}

===== 吹き出しの装飾について @<br>{}
ここで、標準UIの吹き出しの構造を確認しましょう。
ユーザー側の吹き出しには角丸四角の囲み枠があります。
アシスタント側の吹き出しには、囲み枠はありません。左にアイコン、その横に返答メッセージが表示されます。

//image[seahawk-default-bubble-ml][標準の吹き出しデザイン][scale=0.5]

ユーザー側の吹き出しの実装では、囲み枠を非表示にした上で、その内部の枠に装飾を付与しています。
//emlist[public/stylesheet.css][css]{
body:not(
  :has(input[type="password"]) :has(button[type="submit"])
)
  div.px-5.py-2\.5.relative.bg-accent.rounded-3xl
  .max-w-\[70\%\].flex-grow-0
  .message-content {
  background: transparent !important;
  border: 0 !important;
  ...
}
//}

一方、アシスタント側の吹き出しでは囲み枠の削除は不要で、テキスト要素の親要素を装飾すればOKです。ただし、アイコンを含む分、テキスト領域の横幅がユーザーのものよりも短いです。そのため、テキスト領域を広げた上で、アイコンとの横幅の位置調整が必要になります。
//emlist[public/stylesheet.css][css]{
span.relative.flex.shrink-0
  .overflow-hidden.rounded-full[data-state]
  :has(> img[alt="Avatar for Assistant"])
  img[alt="Avatar for Assistant"] {
  ...
  left: -30px;
  ...
}
//}

以上の編集を終えて、静的デザインは完成です。
//image[seahawk-talk-compare-ml][適用後のトーク画面][scale=0.8]


//pagebreak

=== 動的要素を変更する

ここからは、JSファイルとCSSファイルの両方を編集し、吹き出しやロゴに動きをつけていきます。
@<code>{custom_js}の設定は @<hd>{initial_settings} を参照してください。

==== 吹き出しに明滅の演出をつける


メッセージを送信したあとや、アシスタント側の返答が表示されるときにチカチカと2回明滅するような装飾をつけます。

//image[seahawk-blink-animation-flow][明滅アニメーションの概要][scale=1]

そのために、JSで対象要素の監視とクラス付与を行います。@<table>{class-role}にクラスと役割の対応を示します。対象は前項で解説したセレクタで指定します。

//table[class-role][クラスと役割について]{
状態	クラス名	役割
target	@<code>{cl-anim-target}	装飾適用対象の要素につける
active	@<code>{cl-anim-enter-active}	装飾を実際に表示している要素につける
done	@<code>{cl-anim-done}	装飾を表示し終わった要素につける
//}

これらのクラスの付与は@<code>{effects.js}内の@<code>{animateElement}関数で行っています。
//emlist[public/effects.js][javascript]{
  function animateElement(element) {
    ...

    element.dataset[DATA_ATTR.animated] = 'pending';
    element.classList.add(CLASS_NAMES.target);
    element.classList.remove(CLASS_NAMES.done);

    requestAnimationFrame(() => {
      void element.offsetWidth;
      requestAnimationFrame(() => {
        element.classList.add(CLASS_NAMES.active);
      });
    });
    ...
  }
//}

ここまで来れば、あとは@<code>{cl-anim-enter-active}のクラスを対象に、明滅する挙動を定義すれば完成です。

//emlist[public/stylesheet.css][css]{
@keyframes cl-border-blink-twice {
  0% {
    background: var(--chat-frame-border);
  }
  15% {
    background: var(--chat-enter-flash-border);
  }
  ...
}

body:not(
  :has(input[type="password"])
  :has(button[type="submit"])
)
  div.px-5.py-2\.5.relative.bg-accent.rounded-3xl
  .max-w-\[70\%\].flex-grow-0
  .cl-anim-target.cl-anim-enter-active::before,

body:not(
  :has(input[type="password"])
  :has(button[type="submit"])
)
  .message-content:not(
    .px-5.py-2\.5.relative.bg-accent.rounded-3xl
    .max-w-\[70\%\].flex-grow-0
    .message-content
  )
  .cl-anim-target.cl-anim-enter-active::before {
  ...
  animation: cl-border-blink-twice 320ms linear 1;
}
//}

==== ロゴを無駄にぐるぐる回す
最後に、円形モチーフのロゴが回転するアニメーションを実装します。

ロゴは半径・色・太さの異なる複数の円が重なった構造です。(詳細は @<hd>{design} からAppendixをご覧ください) 今回はそれぞれの円を異なるスピードで回転させることにします。すでにそれぞれのパーツを別々のSVGファイルで書き出してありますので、これらのファイルを使って2ステップに分けて実装していきます。
その前に、SVGファイルは、@<code>{public}に@<code>{logos}というディレクトリを作ってまとめて格納しておきます@<fn>{logo-file-url}。


===== ロゴを複数枚のSVGファイルの重ね合わせに置き換える @<br>{}
Chainlitの標準機能では、ロゴは1枚のみ登録可能です(詳細は @<hd>{change-logo} @<fn>{logo-desc} をご覧ください) 。今回は複数のSVGファイルを重ね合わせる必要があるので、JSで標準のロゴ要素を上書きします。このとき、各レイヤーに@<code>{cl-logo-layer-連番}というクラスを付与しておきます。これはのちのCSSのセレクタとして利用します。

//emlist[public/effects.js][javascript]{
/* 重ね合わせロゴのコンテナを作る */
function createLogoContainer(className) {
    const container = document.createElement('div');
    container.className = className;
    container.setAttribute('aria-hidden', 'true');

    LOGO_LAYERS.forEach((src, i) => {
        const layer = document.createElement('img');
        layer.src = src;
        layer.alt = '';
        layer.className = `cl-logo-layer cl-logo-layer-${i + 1}`;
        container.appendChild(layer);
    });

    return container;
}

/* 置き換える */
function replaceLoginLogo() {
    const form = document.querySelector('form:has(input[type="password"])');
    ...
    form.prepend(createLogoContainer('cl-layered-logo'));
    form.dataset.clLogoInjected = 'true';
    ...
}
//}

===== ロゴを回す @<br>{}
上のJSで付与した@<code>{cl-logo-layer-連番}のクラスを使って、それぞれのレイヤーの回転速度を指定します。(@<code>{@keyframe}は省略します)

//emlist[public/stylesheet.css][css]{
:is(.cl-layered-logo) .cl-logo-layer-1 { animation-duration: 16s; }
:is(.cl-layered-logo) .cl-logo-layer-2 { animation-duration: 25s; }
:is(.cl-layered-logo) .cl-logo-layer-3 { animation-duration: 18s; }
...
//}

これでロゴの回転が実装できました。

=== 完成！
以上で動的要素の実装も完成です！完成したUIの挙動を録画したものを @<href>{https://seahawk-tiger.github.io/chainlit-ui-appendix/pages/appendix4-demo-movie.html} で公開しています。QRコードも掲載します。

簡単なアニメーションではありますが、これだけでもちょっとテンションが上がるUIになったと思います。

//image[seahawk-demomovie-qr][完成したUIのデモムービー][scale=0.3]

== まとめ
セレクタを特定してCSSとJSで操作するという地味な作業が多い記事でしたが、フロントエンドの知識が乏しい中でもUIを変更できました。
もっとデザインやフロントエンドの知識があればさらに派手な演出が実装できたかもしれない、と思いつつも、個人的にはこのデザインやアニメーションは気に入っています。
また、一連の作業の中で、Chainlitの標準UIはシンプルで癖がなく、使いやすく設計されていると実感しました。

UIデザインは、プロダクトの利便性の根幹の一つであると同時に、製作者がプロダクトに込めた思いを表現する場でもあります。もし業務やプライベートでChainlitを使うときは、ちょっと凝ったデザインにして、その思いをこっそり色や動きに忍ばせてみてはいかがでしょうか。

== 参考資料
 * Chainlit公式ドキュメント - Customize
 ** https://docs.chainlit.io/customisation/overview
 * Shadcn Documentation - Theming
 ** https://ui.shadcn.com/docs/theming#list-of-variables
 
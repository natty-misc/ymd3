const cheerio = YMD.cheerio;

YMD.log("=====================================");
YMD.log("Initiating retrieval: " + YMD.inputURL);

const originalURL = "https://www.youtube.com/watch?v=" + YMD.inputURL;

const html = YMD.retrieve(originalURL);

const $ = cheerio.load(html);

const embedDataStr = YMD.retrieve(`https://www.youtube.com/oembed?format=json&url=${ encodeURIComponent(originalURL) }`);
const embedData = JSON.parse(embedDataStr);

function getTitle()
{
    return embedData["title"];
}

function getAuthor()
{
    return embedData["author_name"];
}

function getDescrambler(playerConfig)
{
    const playerConfigCode = playerConfig.replaceAll(/\s+/g, "");

    const descramblerPattern = /function\([a-z0-9]+\){([a-z]=[a-z]\.split\(""\);[a-z0-9.,();$]+?return[a-z0-9]\.join\(""\))}/i;

    const match = playerConfigCode.match(descramblerPattern);

    if (!match)
        throw new Error("Could not find the descrambler function!");

    const matchedCode = match[1];

    const matchedCodeSplit = matchedCode.split(";");

    let descramblerCode = matchedCodeSplit
        .slice(1, matchedCodeSplit.length - 1)
        .map(call => {
            const [ fobj, fname, , findex ] = call.split(/[.(),]/);
            return {
                objectName: fobj,
                functionName: fname,
                index: parseInt(findex)
            }
        });

    YMD.log("Descrambler code: ");

    const descramblerObj = descramblerCode[0].objectName;
    YMD.log("  Object: " + descramblerObj);
    YMD.log("");

    const instructionPattern = new RegExp(`var${descramblerObj}={((?:[a-z]+[a-z0-9]*:function\\(.+?\\){.+?};?)+)}`, "i");

    const instructionMatch = playerConfigCode.match(instructionPattern);

    if (!instructionMatch)
        throw new Error("Could not find the descrambler instruction assignment object!");

    const instructionFunctions = instructionMatch[1].split(/},/);

    let instructions = {};

    instructionFunctions.forEach(instruction => {
        const [ name, code ] = instruction.split(":", 2);

        if (code.includes("splice"))
            instructions[name] = "SPLICE";
        else if (code.includes("reverse"))
            instructions[name] = "REVERSE";
        else if (code.includes("%"))
            instructions[name] = "SWAP";
        else
            throw new Error("Unrecognized descrambler instruction: " + code + "}");

    })

    const transformations = descramblerCode.map(call => {
        const opcode = instructions[call.functionName];

        YMD.log("  " + opcode + "   " + call.index);

        switch (opcode)
        {
            case "SWAP":
                return (cipher) => {
                    const tmp = cipher[0];
                    cipher[0] = cipher[call.index % cipher.length];
                    cipher[call.index % cipher.length] = tmp;
                    return cipher;
                }
            case "REVERSE":
                return (cipher) => {
                    return cipher.reverse();
                }
            case "SPLICE":
                return (cipher) => {
                    cipher.splice(0, call.index);
                    return cipher;
                }
        }
    });

    return (cipherText) => {
        let cipher = cipherText.split("");

        for (const transformation of transformations)
            cipher = transformation(cipher);

        return cipher.join("");
    }
}

function getMedia()
{
    const scripts = $("script");

    const scriptBaseURL = "https://www.youtube.com";

    let playerConfigURL = null;
    let videoConfig = null;

    for (const script of scripts)
    {
        const scriptEl = $(script);

        const scriptSrc = scriptEl.attr("src");

        if (/^\/s\/player\/[0-9a-z]+\/player_.+?\/.+?\/base.js/gi.test(scriptSrc))
        {
            playerConfigURL = scriptBaseURL + scriptSrc;
            continue;
        }

        const text = scriptEl.html();

        if (text.includes("var ytInitialPlayerResponse = "))
        {
            const videoConfigJSON = text.match(/{.+}/gi)[0];
            videoConfig = JSON.parse(videoConfigJSON);

            continue;
        }

        if (videoConfig && playerConfigURL)
            break;
    }

    const initialPlayerConfig = YMD.retrieve(playerConfigURL);
    const descrambler = getDescrambler(initialPlayerConfig);

    const { playerConfig, videoDetails, streamingData } = videoConfig;

    const { title, author, viewCount, lengthSeconds, useCipher = false } = videoDetails;

    YMD.log("");
    YMD.log("Video:");
    YMD.log("  Title: " + title);
    YMD.log("  Author: " + author);
    YMD.log("  View count: " + viewCount);
    YMD.log(`  Length: ${lengthSeconds / 60} minutes ${lengthSeconds % 60} seconds`);
    YMD.log("  Cipher: " + useCipher);

    const loudness = playerConfig?.['audioConfig']?.['loudnessDb'];

    if (typeof(loudness) !== "undefined")
    {
        YMD.log("  Loudness: " + loudness + " dB");
    }

    const { formats = [], adaptiveFormats = [] } = streamingData;
    formats.forEach(fmt => fmt["formatType"] = "legacy");
    adaptiveFormats.forEach(fmt => fmt["formatType"] = "adaptive");

    const formatsMerged = [ ...formats, ...adaptiveFormats ];
    let i = 0;
    formatsMerged.forEach(fmt => fmt["key"] = i++);

    for (const format of formatsMerged)
    {
        const {
            key,
            formatType,
            itag,
            width = null,
            height = null,
            mimeType,
            bitrate = "unknown",
            fps = "NaN",
            audioSampleRate = null,
            qualityLabel = null,
            cipher = false,
            signatureCipher = false
        } = format;

        let { url } = format;

        YMD.log("");
        YMD.log(`    Format #${ key }`);
        YMD.log(`    ${ formatType }: ${ qualityLabel || audioSampleRate }`);
        YMD.log("      MIME type: " + mimeType);
        YMD.log("      itag: " + itag);
        YMD.log(`      resolution: ${ width }x${ height }`);
        YMD.log("      bitrate: " + bitrate);
        YMD.log("      fps: " + fps);
        YMD.log("      Cipher: " + cipher);

        if (signatureCipher)
        {
            let params = {};

            for (const param of signatureCipher.split("&"))
            {
                const [key, value] = param.split("=", 2);
                params[key] = decodeURIComponent(value);
            }

            YMD.log("      Mode: signatureCipher");
            YMD.log("      Signature cipher: " + signatureCipher);
            const decodedCipher = descrambler(params["s"]);
            YMD.log("      Decoded cipher: " + decodedCipher);

            url = `${ params["url"] }&${ params["sp"] }=${ encodeURIComponent(decodedCipher) }`;
        }
        else if (cipher)
        {
            YMD.log("      Mode: cipher");
        }
        else
        {
            YMD.log("      Mode: normal");
        }

        YMD.log("      URL: " + url);
        formatsMerged[key]["url"] = url;
    }

    return formatsMerged[formatsMerged.length - 1].url;
}

function getThumbnail()
{
    const thumbnailLink = $("link[rel=image_src]");

    if (thumbnailLink)
        return thumbnailLink.attr("href");
    else
        return null;
}

YMD.log("Thumbnail: " + getThumbnail());

YMD.videoURL = originalURL;
YMD.downloadURL = getMedia();
YMD.videoName = getTitle();
YMD.videoAuthor = getAuthor();
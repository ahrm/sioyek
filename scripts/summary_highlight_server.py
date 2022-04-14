from flask import Flask, request, jsonify
from tqdm import tqdm as q
from transformers import ReformerModelWithLMHead
import re
import torch

def encode(list_of_strings, pad_token_id=0):
    max_length = max([len(string) for string in list_of_strings])

    # create emtpy tensors
    attention_masks = torch.zeros((len(list_of_strings), max_length), dtype=torch.long)
    input_ids = torch.full((len(list_of_strings), max_length), pad_token_id, dtype=torch.long)

    for idx, string in enumerate(list_of_strings):
        # make sure string is in byte format
        if not isinstance(string, bytes):
            string = str.encode(string)

        input_ids[idx, :len(string)] = torch.tensor([x + 2 for x in string])
        attention_masks[idx, :len(string)] = 1

    return input_ids, attention_masks
    
# Decoding
def decode(outputs_ids):
    decoded_outputs = []
    for output_ids in outputs_ids.tolist():
        # transform id back to char IDs < 2 are simply transformed to ""
        decoded_outputs.append("".join([chr(x - 2) if x > 1 else "" for x in output_ids]))
    return decoded_outputs

def roll_encoding(encoding):
    # by default if encodings are smaller than maximum length, they are padded with
    # zeroes at the end. We need the zero padding to be at the begining for next
    # character prediction to work
    new_encoding = torch.clone(encoding)
    num_rows = encoding.shape[0]
    for i in range(num_rows):
        roll_amount = int(len(encoding[i]) - encoding[i].nonzero().max()-1)
        new_encoding[i] = encoding[i].roll(roll_amount)
    return new_encoding

def get_page_queries(page_text, context_size=49):
    res = []
    for i in q(range(1, len(page_text))):
        context = page_text[max(i-context_size, 0): i]
        label = page_text[i]
        res.append((clean_text(context), label))
    return res

def importance_mask(predicted, query):
    # we define important characters to be the ones which are predicted incorrectly
    # given their context
    mask = []
    for pred, q in zip(predicted, query):
        target = q[1]
        if pred[-1] == target:
            mask.append(0)
        else:
            mask.append(1)
    return mask

def batchify(seq, size):
    # we have to batch the data to prevent OOM erros
    res = []
    while seq != []:
        res.append(seq[:size])
        seq = seq[size:]
    return res

def batch_predictor_cuda(texts, batch_size=250):
    
    res = []
    for batch in q(batchify(texts, batch_size)):
        # predict only one character ahead
        max_length = max([len(x) for x in texts]) + 1
        encoded, _ = encode(batch)
        encoded = roll_encoding(encoded)
        generated = model.generate(encoded.to(device), max_length=max_length)
        decoded = decode(generated)
        del encoded
        del generated
        res.extend(decoded)
    return res

def split_text_and_imask(text, imask):
    # it's like text.split(( |\n)), except we split the mask at the same locations too
    text_splits = []
    imask_splits = []
    
    split_locations = [x.start() for x in re.finditer('( |\n)', text)]
    split_locations += [len(text)]
    
    if len(split_locations) > 0:
        text_splits.append(text[:split_locations[0]])
        imask_splits.append(imask[:split_locations[0]])
        for i in range(1, len(split_locations)):
            text_splits.append(text[split_locations[i-1]+1:split_locations[i]])
            imask_splits.append(imask[split_locations[i-1]+1:split_locations[i]])
    else:
        text_splits = [text]
    
    return text_splits, imask_splits

def clean_text(text):
    return "".join(c for c in text if ord(c) < 127)

def refine_imasks(imask_list, fill=True):
    # convert all zeroes before the last one to ones. For example:
    # [1,0,0,1,0,0,0,0] => [1,1,1,1,0,0,0,0]
    # if `fill==True and refine==True`: if there are too much ones, highlight the whole character
    # instead of highlighting most of it
    res = []
    for imask in imask_list:
        try:
            last_index = len(imask) - imask[::-1].index(1)
            if fill and (last_index > len(imask) / 2):
                res.append([1 for x in range(len(imask))])
            else:
                res.append([1 for _ in range(last_index)] + [0 for _ in range(len(imask) - last_index)])
        except ValueError as e:
            # this happens when the mask has no 1s
            res.append(imask)
    return res

def create_result_string(refined_imasks):
    res = ""
    for mask in refined_imasks:
        res = res + "".join([str(x) for x in mask]) + "0"
    return res[:-1]

def get_mask_from_text(text, refine=True, fill=True, context_size=49):
    queries = get_page_queries(text, context_size)
    query_texts = [q[0] for q in queries]
    predicted_results = batch_predictor_cuda(query_texts)
    imask = [1] + importance_mask(predicted_results, queries)
    
    text_splits, imask_splits = split_text_and_imask(text, imask)
    if refine:
        refined_imasks = refine_imasks(imask_splits, fill=fill)
    else:
        refined_imasks = imask_splits
    return create_result_string(refined_imasks)

if __name__ == '__main__':
    device = torch.device('cuda')
    model = ReformerModelWithLMHead.from_pretrained("google/reformer-enwik8").to(device)

    app = Flask(__name__)

    @app.route('/', methods=['POST', 'GET'])
    def handle_text_mask():
        text = request.values.get('text')
        page = request.values.get('page')
        should_refine = int(request.values.get('refine'))
        should_fill = int(request.values.get('fill'))
        context_size = int(request.values.get('context_size'))
        
        refined_imask =  get_mask_from_text(f'{text}', should_refine, should_fill, context_size)
        print('-' * 80)
        print('len(text):', len(text))
        print('len(imask):', len(refined_imask))
            
        return jsonify({'text': refined_imask, 'page': page})

    app.run()
import gutenbergpy.textget
from gutenbergpy.gutenbergcache import GutenbergCache
import gutenbergpy.textget
import os.path
from gutenbergpy.utils import Utils

cache = GutenbergCache.get_cache()
# For the query function you can use the following fields: languages authors types titles subjects publishers bookshelves
res  =cache.query(languages=['en'],type=['Text'],bookshelves=['Poetry',
                                                              'Short Stories',
                                                              'Philosophy',
                                                              'Bestsellers, American, 1895-1923',
                                                              'Humor',
                                                              'Adventure',
                                                              'Folklore',
                                                              'Biology',
                                                              'Mythology'],
                                                  downloadtype=['text/plain','text/plain; charset=iso-8859-1',
                                                                'text/plain; charset=utf-8','text/plain; charset=windows-1251',
                                                                'text/plain; charset=ibm850','text/plain; charset=windows-1252',
                                                                'text/plain; charset=windows-1250','text/plain; charset=utf-16',
                                                                'text/plain; charset=iso-8859-2','text/plain; charset=windows-1253',
                                                                'text/plain; charset=iso-8859-7','text/plain; charset=utf-7'])
total = len(res)
bad_files  = []
good_files = []
for i,id in enumerate(res):
    Utils.update_progress_bar('Getting texts [%d / %d]'%(i,total),i,total)
    sfname = 'file_%d.txt'%(id,)
    fname ='data_raw/%s'%(sfname,)
    try:
        text = gutenbergpy.textget.strip_headers(gutenbergpy.textget.get_text_by_id(id))
        if os.path.isfile(fname) is False:
            text_file = open(fname,'w')
            text_file.write(text)
            text_file.close()
        good_files.append(sfname)
    except:
        bad_files.append(id)

text_file = open('data_raw/error_files.txt', 'w')
for f in bad_files:
    text_file.write(str(f) +'\n')

text_file.close()

text_file = open('data_raw/header.txt', 'w')
text_file.write(str(len(good_files)) +'\n')
for gf in good_files:
    text_file.write(gf +'\n')

text_file.close()
